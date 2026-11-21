#pragma once

// CrossMux Games: a separate firmware (repos/CrossMuxGames) in its own app
// slot, the `games` factory partition (partitions.csv). OTA updates only ever
// target ota_0/ota_1, so they can never overwrite it.
//
// Switching: CrossMux stores which OTA slot it runs from (NVS "crossmux" /
// "return_slot"), makes `games` the boot partition and restarts. Games' "Back
// to Reader" reads return_slot and boots that slot again.
namespace games_slot {

// True when the games slot exists and holds a valid app image. Checked once,
// then cached; always false on boards without the slot.
bool available();

// Save, switch the boot partition to Games and restart. Returns only on
// failure, with CrossMux's own slot still selected for boot.
bool launch();

}  // namespace games_slot
