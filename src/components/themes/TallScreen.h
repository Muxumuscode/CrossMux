#pragma once

// Theme metrics were tuned for 800 px tall portrait screens (X3/X4, Sticky,
// X4 Pro). The LilyGo T5 S3 is 540x960 portrait, so Home layouts built from
// those numbers leave 160+ px empty at the bottom. Each theme picks its
// tall-screen value with tallScreenValue(standard, tall); every other device
// keeps the standard value, so their layouts are unchanged.
//
// These are compile-time per device (like the board profile) because several
// themes feed the same constants into cover-thumbnail generation, cached cover
// paths, and drawing; a runtime switch would let those disagree.
#if defined(FREEINK_DEVICE_LILYGO) && FREEINK_DEVICE_LILYGO
inline constexpr bool kTallPortraitScreen = true;
#else
inline constexpr bool kTallPortraitScreen = false;
#endif

constexpr int tallScreenValue(const int standard, const int tall) { return kTallPortraitScreen ? tall : standard; }
