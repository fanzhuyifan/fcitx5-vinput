#include "gui/utils/config_manager.h"

namespace vinput::gui {

ConfigManager& ConfigManager::Get() {
  static ConfigManager instance;
  return instance;
}

CoreConfig ConfigManager::Load() {
  QMutexLocker locker(&mutex_);
  return LoadCoreConfig();
}

bool ConfigManager::Save(const CoreConfig& config) {
  QMutexLocker locker(&mutex_);
  bool success = SaveCoreConfig(config);
  if (success) {
    emit configChanged();
  }
  return success;
}

} // namespace vinput::gui
