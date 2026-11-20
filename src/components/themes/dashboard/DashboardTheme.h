#pragma once

#include <cstdint>

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
