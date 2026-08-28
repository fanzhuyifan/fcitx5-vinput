#pragma once

#include <memory>
#include <string>

#include "common/asr/model_manager.h"
#include "common/config/core_config.h"

#include "daemon/asr/asr_config.h"
#include "daemon/asr/runtime/recognition_contract.h"

namespace vinput::daemon::asr {

std::unique_ptr<AsrBackend> CreateSherpaOfflineBackend(const CoreConfig& config,
                                                       const LocalAsrProvider& provider,
                                                       std::string* error);

} // namespace vinput::daemon::asr
