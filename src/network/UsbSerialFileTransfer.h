#pragma once

#include <cstddef>

namespace UsbSerialFileTransfer {

// Bounded HTTP exchange during a foreground USB sync command. No passwords
// cross USB. context points to a failure flag, preventing reuse after framing loss.
int relaySyncHttp(void* context, const char* method, const char* endpoint, const char* payload, const char* inputPath,
                  char* response, size_t capacity, const char* outputPath);

enum class ProcessResult {
  None,
  ScreenshotRequested,
  // Developer aid: "CMD:THEME:<n>" asks for UI theme <n> (in memory only) and
  // a fresh Home, so every theme can be captured without touching the device.
  ThemeRequested,
  // Developer aid: "CMD:SLEEPPREVIEW" draws the sleep screen without sleeping,
  // so it can be captured with CMD:SCREENSHOT (USB is off once really asleep).
  SleepPreviewRequested,
  // Developer aid: "CMD:OPEN:<screen>" opens a screen by name (see main.cpp's
  // allowlist) so every layout can be captured without touching the device.
  OpenRequested,
};

ProcessResult process(bool fileTransferAllowed);

// Theme index from the last ThemeRequested result.
int requestedTheme();

// Screen name from the last OpenRequested result.
const char* requestedScreen();

// Register the native USB CDC overflow callback when the X4 Pro transport
// supports it. Other transports intentionally do nothing.
void registerUsbCdcOverflowHandler();

}  // namespace UsbSerialFileTransfer
