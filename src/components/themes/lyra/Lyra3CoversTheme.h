

#pragma once

#include "components/themes/lyra/LyraTheme.h"

class GfxRenderer;

namespace Lyra3CoversMetrics {
constexpr ThemeMetrics values = [] {
  ThemeMetrics v = LyraMetrics::values;
  v.menuRowHeight = tallScreenValue(46, 60);
  // Three covers share the row, so width caps them at the original 226 px
  // even where Lyra's single cover grows on tall screens.
  v.homeCoverHeight = 226;
  v.homeCoverTileHeight = tallScreenValue(300, 320);
  v.homeRecentBooksCount = 3;
  v.keyboardKeyHeight = 56;
  v.keyboardCenteredText = true;
  return v;
}();
}  // namespace Lyra3CoversMetrics

class Lyra3CoversTheme : public LyraTheme {
 public:
  void drawRecentBookCover(GfxRenderer& renderer, Rect rect, const std::vector<RecentBook>& recentBooks,
                           int selectorIndex, bool& coverRendered, bool& coverBufferStored, bool& bufferRestored,
                           const std::function<bool()>& storeCoverBuffer, const BookReadingStats* stats = nullptr,
                           float progressPercent = -1.0f, const GlobalReadingStats* globalStats = nullptr,
                           const char* currentChapterTitle = nullptr) const override;
};
