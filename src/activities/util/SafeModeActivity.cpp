#include "SafeModeActivity.h"

#include <GfxRenderer.h>
#include <I18n.h>
#include <Logging.h>

#include "CrossPointSettings.h"
#include "components/UITheme.h"

#ifndef SIMULATOR
#include <esp_system.h>
#endif

void SafeModeActivity::onEnter() {
  Activity::onEnter();
  message = tr(STR_SAFE_MODE_TITLE);
  message += ' ';
  message += tr(STR_SAFE_MODE_BODY);
  const char* options[] = {tr(STR_SAFE_MODE_CONTINUE), tr(STR_SAFE_MODE_RESET)};
  popup.show(message.c_str(), options, 2, 0, [this](const int idx) {
    if (idx == 1) {
      resetSettingsAndRestart();
      return;
    }
    finish();
  });
  requestUpdate(true);
}

void SafeModeActivity::loop() {
  if (popup.handleInput(mappedInput, [this] { requestUpdate(); })) return;
  finish();  // Dismissed (Back or a tap outside): same as Continue.
}

void SafeModeActivity::render(RenderLock&&) { popup.processRender(renderer, mappedInput); }

void SafeModeActivity::resetSettingsAndRestart() {
  if (!CrossPointSettings::resetSettingsFileForSafeMode()) {
    LOG_ERR("SAFE", "Settings reset failed; continuing in safe mode");
    GUI.drawPopup(renderer, tr(STR_SAFE_MODE_RESET_FAILED));
    finish();
    return;
  }
  GUI.drawPopup(renderer, tr(STR_SAFE_MODE_RESTARTING));
#ifndef SIMULATOR
  // Not silentRestart(): that saves SETTINGS first, which would write the
  // in-memory settings straight back over the reset.
  esp_restart();
#endif
}
