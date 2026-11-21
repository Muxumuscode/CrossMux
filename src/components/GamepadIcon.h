#pragma once

#include <GfxRenderer.h>

#include <algorithm>

// Code-drawn gamepad for the Games entry (UIIcon::Games), so it needs no
// generated bitmap: a rounded body outline, a D-pad on the left and two
// buttons on the right, inside a `size` x `size` box at (x, y).
__attribute__((noinline)) inline void drawGamepadIcon(const GfxRenderer& renderer, const int x, const int y,
                                                      const int size, const bool black) {
  const int bodyH = size * 5 / 8;
  const int top = y + (size - bodyH) / 2;
  const int line = std::max(2, size / 14);
  renderer.drawRoundedRect(x, top, size, bodyH, line, bodyH / 2, black);

  const int cy = top + bodyH / 2;
  const int padCx = x + size * 3 / 10;
  const int arm = std::max(3, size / 7);
  const int bar = std::max(2, size / 11);
  renderer.fillRect(padCx - arm, cy - bar / 2, 2 * arm + 1, bar, black);
  renderer.fillRect(padCx - bar / 2, cy - arm, bar, 2 * arm + 1, black);

  const int button = std::max(3, size / 8);
  const Color color = black ? Color::Black : Color::White;
  renderer.fillRoundedRect(x + size * 64 / 100 - button / 2, cy + size / 16 - button / 2, button, button, button / 2,
                           color);
  renderer.fillRoundedRect(x + size * 78 / 100 - button / 2, cy - size / 16 - button / 2, button, button, button / 2,
                           color);
}
