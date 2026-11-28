# Backlog: recent reading pace drives time left

Status: proposed, not started. Move to a GitHub issue if Issues are enabled.

## Summary

Pages/Min always reads "1.0" because it is a whole-book average
(`pagesTurned * 60 / totalReadingSeconds`, `DashboardTheme.cpp:275`,
`BookStatsView.cpp:283`) shown to one decimal. Time left also has very little
memory of past sessions. Goal: make the reading-speed stats follow how the user
is reading now, without jumping around.

## Findings (current behaviour)

- The Pages/Min formula is correct. Page and time counters accumulate and
  persist correctly (`EpubReaderActivity.cpp:6637`, `ReadingSessionStats.cpp:39`,
  `BookReadingStats.cpp:172/417`).
- Time left, path 1 (stable page numbers): `StableReadingPace` is session-only,
  starts at zero on every open and needs 3 samples (`EpubReaderActivity.cpp:2066`).
- Time left, path 2 (fallback): `ReadingPace::estimateMillis` blends the saved
  pace with the session, and the saved pace has 0% weight after 10 pages
  (`ReadingPace.h:21`). The result overwrites the stored average
  (`EpubReaderActivity.cpp:1979-1982`), so the stored value is really "pace at
  the end of the last session".
- `stats.recordForwardPageRead()` computes a 1000-sample average that is
  immediately overwritten by `setAverageForwardPageMillis`. The comment "Running
  average pace" in `BookReadingStats.h:15` is misleading.
- Existing guards: pages under 2 s ignored, pages over the idle threshold
  (default 5 min) ignored, 3-sample warm-up, back-turn pages skipped.

## Proposed work

1. **Pages/Min decimals:** two decimals on Dashboard and Book Stats
   (`DashboardTheme.cpp:373`, `BookStatsView.cpp:405/483`).
2. **One shared "recent pace":** exponential moving average. Each accepted page
   moves the pace about 1/25 toward that page's time (tune on device; 1/15 reacts
   faster, 1/40 is steadier). Persist it between sessions and replace the
   10-page handover. Reuses the existing pace field, so no format change for
   this part.
3. **Outlier clamp:** clamp each page's time to 1/3x..3x the current pace before
   it counts, on top of the existing 2 s and idle-threshold limits.
4. **Keep the 3-page warm-up** and keep showing the previous estimate until it
   completes.
5. **Display hysteresis:** only update the shown time left when it moves more
   than 5% or 1 minute, whichever is larger.
6. **Persist path 1's pace** (source words per second) so `StableReadingPace`
   stops resetting on every open. Needs a new field: bump the stats format v7 to
   v8 and document it in `docs/file-formats.md`.
7. **Dashboard readout:** in the free space next to the title, show recent pace
   as pages/min plus an up/down arrow against the book's lifetime average. Show
   the arrow only when they differ by more than 10%. Hide the whole readout when
   the title is long enough to need the space. Dashboard theme only.
8. **Time left everywhere uses the recent pace:** the Dashboard "Time left" row
   and the sleep screen number both come from it. No new element and no label
   change on the sleep screen.
9. **Cleanup:** drop the overwritten 1000-page average and fix the
   `BookReadingStats.h:15` comment.

## Decisions and constraints

- Show the recent pace (item 2) next to the title, not the raw last-session
  rate, so the rate, the arrow and the time left always agree.
- Nothing else new on the sleep screen.
- Old books have no recent-pace history, so the readout appears only after the
  first new session.
- C3-safe: no new heap, a few bytes of state.

## Open questions

- Adapt rate (1/25 as a starting point).
- Whether to include item 6 and its format bump (recommended; without it path 1
  books still reset on every open).

## Hardware verification (when built)

Read a book for about 30 pages at a steady pace and note the time left. Then
read 5 pages fast, or let one page sit idle: the estimate should drift, not
jump. Close and reopen the book: it should resume near the same value instead
of recalculating from zero. Clear the book's `.crosspoint/epub_<hash>/` cache
only if layout behaviour looks stale.
