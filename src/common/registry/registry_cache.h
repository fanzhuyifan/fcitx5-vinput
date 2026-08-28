#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "common/utils/downloader.h"

namespace vinput::registry::cache {

struct FetchStatus {
  bool used_cache = false;
  std::string fallback_error;
};

bool FetchText(const std::vector<std::string>& urls, const std::filesystem::path& cache_path,
               const vinput::download::Options& options, std::string* content,
               vinput::download::Result* result = nullptr, std::string* error = nullptr,
               FetchStatus* status = nullptr);

std::filesystem::path ModelRegistryPath();
std::filesystem::path AsrProviderRegistryPath();
std::filesystem::path LlmAdapterRegistryPath();
std::filesystem::path I18nPath(const std::string& locale);

} // namespace vinput::registry::cache
