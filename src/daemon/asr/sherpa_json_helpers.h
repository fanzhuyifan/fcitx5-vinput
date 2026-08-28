#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <string_view>

namespace vinput::daemon::asr {

inline float SafeStof(const std::string& s, float default_val) {
  try {
    return std::stof(s);
  } catch (...) {
    return default_val;
  }
}

inline int SafeStoi(const std::string& s, int default_val) {
  try {
    return std::stoi(s);
  } catch (...) {
    return default_val;
  }
}

inline std::string JsonString(const nlohmann::json& obj, std::string_view key,
                              const std::string& default_val = {}) {
  if (!obj.is_object() || !obj.contains(key) || !obj[key].is_string()) {
    return default_val;
  }
  return obj[key].get<std::string>();
}

inline int JsonInt(const nlohmann::json& obj, std::string_view key, int default_val) {
  if (!obj.is_object() || !obj.contains(key)) {
    return default_val;
  }
  const auto& value = obj[key];
  if (value.is_number_integer()) {
    return value.get<int>();
  }
  if (value.is_boolean()) {
    return value.get<bool>() ? 1 : 0;
  }
  if (value.is_string()) {
    return SafeStoi(value.get<std::string>(), default_val);
  }
  return default_val;
}

inline float JsonFloat(const nlohmann::json& obj, std::string_view key, float default_val) {
  if (!obj.is_object() || !obj.contains(key)) {
    return default_val;
  }
  const auto& value = obj[key];
  if (value.is_number()) {
    return value.get<float>();
  }
  if (value.is_string()) {
    return SafeStof(value.get<std::string>(), default_val);
  }
  return default_val;
}

inline bool JsonBool(const nlohmann::json& obj, std::string_view key, bool default_val = false) {
  if (!obj.is_object() || !obj.contains(key)) {
    return default_val;
  }
  const auto& value = obj[key];
  if (value.is_boolean()) {
    return value.get<bool>();
  }
  if (value.is_number_integer()) {
    return value.get<int>() != 0;
  }
  if (value.is_string()) {
    const auto raw = value.get<std::string>();
    return raw == "true" || raw == "1";
  }
  return default_val;
}

} // namespace vinput::daemon::asr
