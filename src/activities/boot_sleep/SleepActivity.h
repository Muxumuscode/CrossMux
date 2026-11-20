#pragma once
#include <string>
#include <utility>

#include "activities/Activity.h"
#include "activities/reader/BookReadingStats.h"

class Bitmap;

class SleepActivity final : public Activity {
 public:
  explicit SleepActivity(GfxRenderer& renderer, MappedInputManager& mappedInput, bool canSnapshotOverlayBackground,
                         std::string currentBookPath = {}, bool fromTimeout = false,
                         GfxRenderer::Orientation sleepPopupOrientation = GfxRenderer::Orientation::Portrait)
      : Activity("Sleep", renderer, mappedInput),
        canSnapshotOverlayBackground(canSnapshotOverlayBackground),
        currentBookPath(std::move(currentBookPath)),
        fromTimeout(fromTimeout),
        sleepPopupOrientation(sleepPopupOrientation) {}
  void onEnter() override;
  void showSleepPopup();
  // Whether this screen is independent of the outgoing activity's saves.
  bool rendersBeforeExit() const;
  bool hasMonochromeFrame() const { return monochromeFrame; }

 private:
#ifdef SIMULATOR
  friend struct UpstreamFeatureSmokeTest;
#endif
  void renderDefaultSleepScreen() const;
  // Current-book info for the "Custom + Reading Strip" mode, loaded once before
  // the image is drawn. The strip spans the screen width from `top`; a zero
  // `height` means no strip (no book yet), so the image gets the full screen.
  struct ReadingStrip {
    int top = 0;
    int height = 0;
    BookReadingStats stats;
    bool hasStats = false;
    float progressPercent = -1.0f;
    std::string title;
    std::string chapterTitle;
    uint32_t todaySeconds = 0;
    uint16_t streakDays = 0;
  };
  void renderCustomSleepScreen(bool withReadingStrip = false) const;
  bool renderCustomSleepBmp(const std::string& path, const ReadingStrip& strip) const;
  ReadingStrip loadReadingStrip() const;
  void renderCoverSleepScreen() const;
  void renderReadingStatsSleepScreen() const;
  void renderMinimalSleepScreen() const;
  void renderMinimalStatsSleepScreen() const;
  void renderDashboardSleepScreen() const;
  struct BitmapPlacement {
    int x, y, width, height;
    float cropX, cropY;
  };
  // `strip` (optional) reserves the bottom band and is drawn in the B/W pass.
  BitmapPlacement prepareSleepBitmap(Bitmap& bitmap, const ReadingStrip* strip = nullptr) const;
  bool drawSleepBitmap(Bitmap& bitmap, const BitmapPlacement& placement, const ReadingStrip* strip = nullptr) const;
  bool renderBitmapSleepScreen(Bitmap& bitmap, const ReadingStrip* strip = nullptr) const;
  void renderLastScreenSleepScreen() const;
  void renderBlankSleepScreen() const;
  void renderOverlaySleepScreen(Bitmap* cover = nullptr) const;
  mutable bool monochromeFrame = false;
  bool canSnapshotOverlayBackground = false;
  bool overlayBackgroundAvailable = false;
  bool sleepPopupShown = false;
  std::string currentBookPath;
  bool fromTimeout = false;
  GfxRenderer::Orientation sleepPopupOrientation = GfxRenderer::Orientation::Portrait;
};
