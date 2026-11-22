#include "GamesSlot.h"

#include <Logging.h>

#include "SilentRestart.h"

#if defined(FREEINK_DEVICE_LILYGO) && FREEINK_DEVICE_LILYGO && !defined(SIMULATOR)
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <nvs.h>

namespace games_slot {
namespace {

const esp_partition_t* gamesPartition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, "games");
}

esp_app_desc_t gDesc{};
int cached = -1;

}  // namespace

void refresh() { cached = -1; }

Status status() {
  if (cached < 0) {
    const esp_partition_t* games = gamesPartition();
    // Reads and checks the app descriptor's magic: an erased or partially
    // written slot fails here, so CrossMux never offers to boot it.
    if (!games) {
      cached = static_cast<int>(Status::NoSlot);
      LOG_INF("GAMES", "No games slot in this partition table (needs a USB update)");
    } else if (esp_ota_get_partition_description(games, &gDesc) == ESP_OK) {
      cached = static_cast<int>(Status::Installed);
      LOG_INF("GAMES", "Games slot: %s %s", gDesc.project_name, gDesc.version);
    } else {
      cached = static_cast<int>(Status::Empty);
      LOG_INF("GAMES", "Games slot is empty");
    }
  }
  return static_cast<Status>(cached);
}

const char* version() { return status() == Status::Installed ? gDesc.version : ""; }

bool launch() {
  const esp_partition_t* games = gamesPartition();
  const esp_partition_t* running = esp_ota_get_running_partition();
  if (!available() || !games || !running) return false;
  if (running->subtype < ESP_PARTITION_SUBTYPE_APP_OTA_0 || running->subtype > ESP_PARTITION_SUBTYPE_APP_OTA_1) {
    LOG_ERR("GAMES", "CrossMux is not running from an OTA slot");
    return false;
  }

  nvs_handle_t handle;
  if (nvs_open("crossmux", NVS_READWRITE, &handle) != ESP_OK) {
    LOG_ERR("GAMES", "Could not open NVS for the return slot");
    return false;
  }
  const uint8_t slot = static_cast<uint8_t>(running->subtype - ESP_PARTITION_SUBTYPE_APP_OTA_0);
  const bool stored = nvs_set_u8(handle, "return_slot", slot) == ESP_OK && nvs_commit(handle) == ESP_OK;
  nvs_close(handle);
  if (!stored) {
    LOG_ERR("GAMES", "Could not store the return slot");
    return false;
  }

  if (esp_ota_set_boot_partition(games) != ESP_OK) {
    LOG_ERR("GAMES", "Could not select the games slot for boot");
    return false;
  }
  LOG_INF("GAMES", "Switching to Games (return to ota_%u)", static_cast<unsigned>(slot));
  // silentRestart() flushes settings and storage like any other restart.
  if (silentRestart()) return true;

  // Restart refused (deep sleep in progress or a failed flush): boot CrossMux
  // next time, not Games.
  esp_ota_set_boot_partition(running);
  return false;
}

}  // namespace games_slot

#else

namespace games_slot {
Status status() { return Status::NoSlot; }
void refresh() {}
const char* version() { return ""; }
bool launch() { return false; }
}  // namespace games_slot

#endif
