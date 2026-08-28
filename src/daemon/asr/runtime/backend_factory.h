#pragma once

#include <memory>
#include <string>

#include "common/config/core_config.h"

#include "daemon/asr/runtime/recognition_contract.h"

namespace vinput::daemon::asr {

std::unique_ptr<AsrBackend> CreateBackend(const CoreConfig& config, std::string* error);
bool DescribeActiveBackend(const CoreConfig& config, BackendDescriptor* descriptor,
                           std::string* error);

} // namespace vinput::daemon::asr
