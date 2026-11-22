#pragma once

#include <cstdint>
#include <string>

class GfxRenderer;

// One calendar year of reading on this device, for the Year in Reading page
// of Reading Stats. From January 1-14 it covers the previous year, otherwise the year so
// far. Everything comes from data the reader already keeps: the daily reading
// counters, the two-year reading calendar, per-book finished dates and the
// clipping store. Synced stats from other devices are not included.
struct YearInReading {
  uint16_t year = 0;
  uint32_t booksFinished = 0;
  uint32_t readingSeconds = 0;
  uint16_t daysRead = 0;
  uint16_t longestStreak = 0;
  uint32_t highlights = 0;
  std::string topBookTitle;
  std::string topBookAuthor;
  uint16_t topBookHighlights = 0;
  std::string quote;
  std::string quoteBookTitle;

  // False without a set clock: there is no year to report.
  bool load();
  // Draws the stats, top book and quote inside the given area.
  void draw(const GfxRenderer& renderer, int x, int top, int width, int height) const;
};
