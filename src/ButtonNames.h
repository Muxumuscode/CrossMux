#pragma once

#include <I18n.h>

#include "DeviceCapabilities.h"

// Controls and Quick Actions name the keys the way the device prints them. The
// LilyGo T5's readable keys are BOOT (the power key) and IO48 (its only side key).
inline constexpr StrId powerButtonNameId() {
  return deviceUsesTwoKeyNavigation() ? StrId::STR_BOOT_BUTTON : StrId::STR_POWER_BUTTON;
}
inline constexpr StrId sideButtonsNameId() {
  return deviceUsesTwoKeyNavigation() ? StrId::STR_IO48_BUTTON : StrId::STR_SIDE_BUTTONS;
}
inline constexpr StrId powerChordNameId() {
  return deviceUsesTwoKeyNavigation() ? StrId::STR_BOOT_IO48_CHORD : StrId::STR_POWER_BUTTON_CHORD;
}
inline constexpr StrId shortPressPowerNameId() {
  return deviceUsesTwoKeyNavigation() ? StrId::STR_SHORT_PRESS_BOOT : StrId::STR_SHORT_PRESS_POWER;
}
inline constexpr StrId longPressPowerNameId() {
  return deviceUsesTwoKeyNavigation() ? StrId::STR_LONG_PRESS_BOOT : StrId::STR_LONG_PRESS_POWER;
}
