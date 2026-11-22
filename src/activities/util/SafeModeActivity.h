#pragma once
#include <string>

#include "activities/Activity.h"
#include "components/OptionPopup.h"

// Shown once over Home after a safe-mode boot (T5: wake with BOOT while
// holding IO48). The boot already skipped reopening the last book and turned
// plugins off for this session; this screen says so and offers a settings
// reset, which keeps the old file on the SD card and restarts.
class SafeModeActivity final : public Activity {
  std::string message;
  OptionPopup popup;

 public:
  SafeModeActivity(GfxRenderer& renderer, MappedInputManager& mappedInput) : Activity("SafeMode", renderer, mappedInput) {}
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  void resetSettingsAndRestart();
};
