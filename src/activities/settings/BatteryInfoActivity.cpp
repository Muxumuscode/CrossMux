#include "BatteryInfoActivity.h"

#include <I18n.h>
#include <Logging.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>

#include "MappedInputManager.h"
#include "components/TouchHeaderBackButton.h"
#include "components/UITheme.h"
#include "components/UiAppHelpers.h"

namespace fui = freeink::ui;
namespace {
// The gauge averages current over ~1 s and updates SoC slowly; a few seconds
// between reads keeps the panel calm and still shows a load change quickly.
constexpr unsigned long kRefreshMs = 5000;

enum Row { Charge, State, Current, TimeRow, Voltage, Capacity, Design, Health, Cycles, Temperature, RowCount };
constexpr StrId labels[] = {StrId::STR_BATTERY_CHARGE,   StrId::STR_BATTERY_STATE,    StrId::STR_BATTERY_CURRENT,
                            StrId::STR_BATTERY_TIME,     StrId::STR_BATTERY_VOLTAGE,  StrId::STR_BATTERY_CAPACITY,
                            StrId::STR_BATTERY_DESIGN,   StrId::STR_BATTERY_HEALTH,   StrId::STR_BATTERY_CYCLES,
                            StrId::STR_BATTERY_TEMPERATURE};
static_assert(sizeof(labels) / sizeof(labels[0]) == RowCount);

void formatMinutes(char* buf, const size_t size, const uint16_t minutes) {
  if (minutes >= 60) {
    snprintf(buf, size, "%uh %02um", static_cast<unsigned>(minutes / 60), static_cast<unsigned>(minutes % 60));
  } else {
    snprintf(buf, size, "%um", static_cast<unsigned>(minutes));
  }
}
}  // namespace

BatteryInfoActivity::BatteryInfoActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("BatteryInfo", renderer, mappedInput),
      uiTarget(makeUiTarget(renderer)),
      app(uiTarget, uiTarget.deviceContext()) {}

void BatteryInfoActivity::readGauge() {
  lastReadMs = millis();
  if (!BatteryMonitor::readGaugeDetails(details)) {
    LOG_ERR("BATT", "Gauge detail read failed");
  }
  const BatteryMonitor battery;
  const auto status = battery.readStatus();
  charging = status.chargingKnown && status.charging;
  externalPowerKnown = status.externalPowerKnown;
  externalPower = status.externalPower;
}

void BatteryInfoActivity::onEnter() {
  Activity::onEnter();
  readGauge();
  applySharedUiTheme(app, uiTarget);
  app.setScreen(&BatteryInfoActivity::batteryScreen, this);
  requestUpdate();
}

void BatteryInfoActivity::loop() {
  RenderLock lock(*this);  // Protect the readings shared with the render task.
  if (TouchHeaderBackButton::wasTapped(mappedInput, renderer) ||
      mappedInput.wasPressed(MappedInputManager::Button::Back)) {
    finishAfterBackPress();
    return;
  }
  const auto scroll = [this](int delta) {
    const int next = scrollListBy(topIndex, delta, visibleRows, RowCount);
    if (next != topIndex) {
      topIndex = next;
      requestUpdate();
    }
  };
  const auto swipe = mappedInput.wasSwipe();
  if (swipe == MappedInputManager::SwipeDir::Up || swipe == MappedInputManager::SwipeDir::Down) {
    scroll(swipe == MappedInputManager::SwipeDir::Up ? visibleRows : -visibleRows);
    return;
  }
  buttonNavigator.onNextRelease([&] { scroll(visibleRows); });
  buttonNavigator.onPreviousRelease([&] { scroll(-visibleRows); });

  if (millis() - lastReadMs >= kRefreshMs) {
    readGauge();
    requestUpdate();
  }
}

void BatteryInfoActivity::provideRow(void* user, const uint16_t index, fui::ListItem& item) {
  auto& self = *static_cast<BatteryInfoActivity*>(user);
  const auto& d = self.details;
  auto* buf = self.valueBuffer;
  const auto size = sizeof(self.valueBuffer);
  item.label = I18N.get(labels[index]);
  item.subtitle = tr(STR_UNAVAILABLE);
  if (!d.valid && index != State) return;
  switch (index) {
    case Charge:
      snprintf(buf, size, "%u%% (%u / %u mAh)", static_cast<unsigned>(d.percentage),
               static_cast<unsigned>(d.remainingMah), static_cast<unsigned>(d.fullChargeMah));
      item.subtitle = buf;
      break;
    case State:
      if (self.charging) {
        item.subtitle = tr(STR_BATTERY_CHARGING);
      } else if (self.externalPowerKnown && self.externalPower) {
        item.subtitle = tr(STR_BATTERY_PLUGGED_IN);
      } else {
        item.subtitle = tr(STR_BATTERY_ON_BATTERY);
      }
      break;
    case Current:
      // The gauge reports + for charging; show draw as a positive number.
      snprintf(buf, size, d.averageCurrentMa > 0 ? "+%d mA" : "%d mA",
               d.averageCurrentMa > 0 ? d.averageCurrentMa : std::abs(d.averageCurrentMa));
      item.subtitle = buf;
      break;
    case TimeRow: {
      char t[16];
      if (d.timeToFullMin != 0xFFFF && d.averageCurrentMa > 0) {
        formatMinutes(t, sizeof(t), d.timeToFullMin);
        snprintf(buf, size, tr(STR_BATTERY_TO_FULL_FORMAT), t);
        item.subtitle = buf;
      } else if (d.timeToEmptyMin != 0xFFFF) {
        formatMinutes(t, sizeof(t), d.timeToEmptyMin);
        snprintf(buf, size, tr(STR_BATTERY_LEFT_FORMAT), t);
        item.subtitle = buf;
      }
      break;
    }
    case Voltage:
      snprintf(buf, size, "%u.%02u V", static_cast<unsigned>(d.millivolts / 1000),
               static_cast<unsigned>((d.millivolts % 1000) / 10));
      item.subtitle = buf;
      break;
    case Capacity:
      snprintf(buf, size, "%u mAh", static_cast<unsigned>(d.fullChargeMah));
      item.subtitle = buf;
      break;
    case Design:
      snprintf(buf, size, "%u mAh", static_cast<unsigned>(d.designMah));
      item.subtitle = buf;
      break;
    case Health:
      if (d.designMah > 0) {
        // The gauge's own SoH plus the learned/design ratio it is based on.
        snprintf(buf, size, tr(STR_BATTERY_HEALTH_FORMAT), static_cast<unsigned>(d.healthPercent),
                 static_cast<unsigned>((static_cast<uint32_t>(d.fullChargeMah) * 100u) / d.designMah));
      } else {
        snprintf(buf, size, "%u%%", static_cast<unsigned>(d.healthPercent));
      }
      item.subtitle = buf;
      break;
    case Cycles:
      snprintf(buf, size, "%u", static_cast<unsigned>(d.cycleCount));
      item.subtitle = buf;
      break;
    case Temperature: {
      const int t = d.temperatureDeciC;
      snprintf(buf, size, "%s%d.%d C", t < 0 ? "-" : "", std::abs(t) / 10, std::abs(t) % 10);
      item.subtitle = buf;
      break;
    }
  }
}

void BatteryInfoActivity::batteryScreen(UiApp::ScreenType& screen, void* user) {
  auto& self = *static_cast<BatteryInfoActivity*>(user);
  const auto& metrics = UITheme::getInstance().getMetrics();
  const Rect header = TouchHeaderBackButton::headerRect(self.renderer, self.mappedInput);
  const auto safe = screen.frame().safeRect();
  const Rect hints =
      UITheme::getInstance().getScreenSafeArea(self.renderer, !self.mappedInput.hasTouchHardware(), false);
  const int contentTop = std::max({static_cast<int>(safe.y), header.y + header.height, hints.y});
  screen.setContentMargin(fui::Insets{static_cast<int16_t>(contentTop - safe.y),
                                      static_cast<int16_t>(std::max<int>(0, safe.right() - (hints.x + hints.width))),
                                      static_cast<int16_t>(std::max<int>(0, safe.bottom() - (hints.y + hints.height))),
                                      static_cast<int16_t>(std::max<int>(0, hints.x - safe.x))});
  screen.spacer(static_cast<int16_t>(metrics.verticalSpacing));
  fui::ListProps props;
  props.count = RowCount;
  props.rowProvider = &BatteryInfoActivity::provideRow;
  props.rowProviderCtx = &self;
  props.inputMask = fui::InputNone;
  props.labelText = screen.theme().bodyText;
  props.labelText.bold = true;
  props.labelText.maxLines = 1;
  props.subtitleText = screen.theme().smallText;
  props.subtitleText.bold = false;
  props.subtitleText.maxLines = 1;
  props.rowPaddingY = screen.theme().spaceSm;
  self.visibleRows =
      std::max<int>(1, configureUiList(props, screen.theme(), screen.body(), UiListRowType::WithSubtitle));
  self.topIndex = scrollListBy(self.topIndex, 0, self.visibleRows, RowCount);
  props.topIndex = self.topIndex;
  screen.list(props);
}

void BatteryInfoActivity::render(RenderLock&&) {
  renderer.clearScreen();
  const Rect header = TouchHeaderBackButton::headerRect(renderer, mappedInput);
  if (mappedInput.hasTouchHardware())
    TouchHeaderBackButton::draw(renderer, uiTarget, header, tr(STR_BATTERY), false);
  else
    GUI.drawHeader(renderer, header, tr(STR_BATTERY));
  renderUiApp(app, uiTarget);
  const auto labels =
      mappedInput.mapLabels(mappedInput.withBackArrow(tr(STR_BACK)), "", tr(STR_DIR_UP), tr(STR_DIR_DOWN));
  GUI.drawButtonHints(renderer, labels.btn1, labels.btn2, labels.btn3, labels.btn4);
  renderer.displayBuffer();
}
