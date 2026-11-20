#pragma once

#include <cstdint>

#include "AppCapabilities.h"
#include "components/themes/minimal/MinimalTheme.h"

namespace DashboardMetrics {
constexpr ThemeMetrics makeValues() {
  ThemeMetrics v = MinimalMetrics::values;
  v.homeTopPadding = 50;
  v.homeCoverHeight = 445;
  // 2.0 lays Dashboard out inside this tile: on the T5 it runs to the screen
  // bottom (50 + 910 = 960) so the footer, totals, and calendar fit below the cover.
  v.homeCoverTileHeight = tallScreenValue(690, 910);
  v.homeRecentBooksCount = 1;
  v.homeContinueReadingInMenu = false;
  v.homeMenuTopOffset = 0;
  return v;
}

constexpr ThemeMetrics values = makeValues();
// One size feeds the Home thumbnail generator, the Dashboard layout, and the
// Dashboard sleep-cover cache, so they always agree on the cached BMP path.
#if defined(FREEINK_DEVICE_LILYGO) && FREEINK_DEVICE_LILYGO
// LilyGo T5 S3: 540x960 portrait has ~160 px more height than the X4. Grow the
// 2:3 cover into it while leaving ~19 px before the right-aligned stats labels.
constexpr int homeCoverImageWidth = 360;
constexpr int homeCoverImageHeight = 540;
#else
constexpr int homeCoverImageWidth = 296;
constexpr int homeCoverImageHeight = 444;
#endif
}  // namespace DashboardMetrics

class DashboardTheme : public MinimalTheme {
 public:
  // Tall touch screens (the LilyGo T5) show a row of shortcut icons at the
  // bottom of Home, because they have no front-button hints. Compile-time, like
  // TallScreen.h: Home's cover geometry (and so its cached thumbnail size)
  // depends on whether the dock reserves space.
  static constexpr bool hasHomeDock() { return kTallPortraitScreen && CROSSINK_APP_CAP_TOUCH; }
  static constexpr int kHomeDockMaxItems = 8;
  // Draws one icon tile per entry and registers each as TouchRegistry::Item
  // with its index as the id. `pressedIndex` (or -1) draws as highlighted.
  static void drawHomeDock(const GfxRenderer& renderer, const UIIcon* icons, int count, int pressedIndex);

  // "Custom + Reading Strip" sleep screen: a white strip under the custom image.
  // Row 1: book title (bold) and chapter. Row 2: progress, time left, estimated
  // finish, today's reading, and streak, dropping the last items that don't fit.
  // 60 px lets an X4-sized 480x800 image scale to exactly 540x900 above it.
  // 540 - 480x800 scaled to the T5's width (540x900) leaves exactly 60 px, so
  // custom images stay flush on three sides: one line in the largest face that
  // fits (title; time left, streak, progress %) above a 4 px progress bar.
  static constexpr int kSleepReadingStripHeight = 60;
  struct SleepStripInfo {
    const char* title = nullptr;
    const BookReadingStats* stats = nullptr;  // null: no Time Left
    float progressPercent = -1.0f;
    uint16_t streakDays = 0;  // 0: omitted
  };
  static void drawSleepReadingStrip(const GfxRenderer& renderer, const Rect& strip, const SleepStripInfo& info);

  static Rect getHomeCoverRect(const GfxRenderer& renderer);
  static Rect getSleepCoverRect(const GfxRenderer& renderer);

  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           const std::function<bool()>& storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f, const GlobalReadingStats* globalStats = nullptr,
                           const char* currentChapterTitle = nullptr) const override;
  void drawSleepScreen(const GfxRenderer& renderer, const RecentBook& book, const BookReadingStats* stats,
                       const GlobalReadingStats* globalStats, float progressPercent = -1.0f,
                       const char* currentChapterTitle = nullptr, bool inverted = false) const;
};
