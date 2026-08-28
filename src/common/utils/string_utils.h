#pragma once
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>
#include <string_view>
#include <type_traits>

namespace vinput::str {

namespace detail {
template <typename T> auto ToCArg(const T& v) {
  if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
    return v.c_str();
  } else {
    return v;
  }
}
} // namespace detail

template <typename... Args> std::string FmtStr(const char* fmt, const Args&... args) {
  int n = std::snprintf(nullptr, 0, fmt, detail::ToCArg(args)...);
  if (n < 0)
    return fmt;
  std::string buf(static_cast<size_t>(n) + 1, '\0');
  std::snprintf(buf.data(), buf.size(), fmt, detail::ToCArg(args)...);
  buf.resize(static_cast<size_t>(n));
  return buf;
}

inline std::string FormatSize(uint64_t bytes) {
  if (bytes >= 1024ULL * 1024 * 1024)
    return FmtStr("%.1f GB", bytes / (1024.0 * 1024 * 1024));
  if (bytes >= 1024ULL * 1024)
    return FmtStr("%.1f MB", bytes / (1024.0 * 1024));
  if (bytes >= 1024)
    return FmtStr("%.1f KB", bytes / 1024.0);
  return FmtStr("%llu B", (unsigned long long)bytes);
}

inline std::string TrimAsciiWhitespace(std::string_view text) {
  size_t begin = 0;
  while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin]))) {
    begin++;
  }
  size_t end = text.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    end--;
  }
  return std::string(text.substr(begin, end - begin));
}

inline bool IsCjkCodepoint(uint32_t cp) {
  return cp >= 0x2E80;
}

inline uint32_t FirstUtf8Codepoint(std::string_view s) {
  if (s.empty())
    return 0;
  auto c = static_cast<unsigned char>(s[0]);
  if (c < 0x80)
    return c;
  uint32_t cp = 0;
  int len = 0;
  if ((c & 0xE0) == 0xC0) {
    cp = c & 0x1F;
    len = 2;
  } else if ((c & 0xF0) == 0xE0) {
    cp = c & 0x0F;
    len = 3;
  } else if ((c & 0xF8) == 0xF0) {
    cp = c & 0x07;
    len = 4;
  } else {
    return c;
  }
  for (int i = 1; i < len && static_cast<size_t>(i) < s.size(); ++i) {
    cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
  }
  return cp;
}

inline uint32_t LastUtf8Codepoint(std::string_view s) {
  if (s.empty())
    return 0;
  auto i = s.size();
  while (i > 0 && (static_cast<unsigned char>(s[i - 1]) & 0xC0) == 0x80) {
    --i;
  }
  if (i == 0)
    return 0;
  return FirstUtf8Codepoint(s.substr(i - 1));
}

inline bool IsSentenceEndingPunctuation(uint32_t cp) {
  // CJK sentence enders: 。！？…
  if (cp == 0x3002 || cp == 0xFF01 || cp == 0xFF1F || cp == 0x2026)
    return true;
  // Latin sentence enders
  if (cp == '.' || cp == '!' || cp == '?')
    return true;
  if (cp == '\n')
    return true;
  return false;
}

} // namespace vinput::str
