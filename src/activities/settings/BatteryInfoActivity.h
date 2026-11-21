#pragma once

#include <BatteryMonitor.h>
#include <FreeInkApp.h>
#include <FreeInkUIGfxRenderer.h>

#include "activities/Activity.h"
#include "util/ButtonNavigator.h"

// Settings > System > Battery: live fuel-gauge readings (BQ27220 boards, the
// LilyGo T5): charge, voltage, current draw, time left or to full, learned vs
// design capacity, health, cycle count, and temperature. Re-reads the gauge
// every few seconds while open; nothing is written.
class BatteryInfoActivity final : public Activity {
 public:
  BatteryInfoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  using UiApp = freeink::ui::FreeInkApp<1, 1>;
  ButtonNavigator buttonNavigator;
  freeink::ui::GfxRendererTarget uiTarget;
  UiApp app;
  BatteryMonitor::GaugeDetails details;
  bool charging = false;
  bool externalPower = false;
  bool externalPowerKnown = false;
  unsigned long lastReadMs = 0;
  // Reused by the render-only row provider; no strings per row or frame.
  char valueBuffer[64]{};
  int topIndex = 0;
  int visibleRows = 1;
  void readGauge();
  static void batteryScreen(UiApp::ScreenType& screen, void* user);
  static void provideRow(void* user, uint16_t index, freeink::ui::ListItem& item);
};
