#pragma once

#include <chrono>
#include <cstdint>

namespace vinput::runtime {

inline constexpr uint64_t kDbusCallTimeoutUsec = 5ULL * 1000ULL * 1000ULL;
inline constexpr int kDbusCallTimeoutMs = 5000;
inline constexpr auto kDaemonFailureCooldown = std::chrono::milliseconds(1500);
inline constexpr int kInfoNotificationTimeoutMs = 3000;
inline constexpr int kErrorNotificationTimeoutMs = 5000;

} // namespace vinput::runtime
