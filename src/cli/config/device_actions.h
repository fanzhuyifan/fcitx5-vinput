#pragma once

#include <string>

#include "cli/utils/cli_context.h"
#include "cli/utils/formatter.h"

int RunDeviceConfigList(Formatter& fmt, const CliContext& ctx);
int RunDeviceConfigUse(const std::string& name, Formatter& fmt, const CliContext& ctx);
