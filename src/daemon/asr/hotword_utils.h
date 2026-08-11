#pragma once

#include <string>
#include <string_view>

struct ModelInfo;

namespace vinput::daemon::asr {

bool IsTransducerHotwordFamily(std::string_view family);
bool IsPromptHotwordFamily(std::string_view family);

// Returns whether the configured file contains at least one non-whitespace
// line. Missing or unreadable configured files are errors.
bool HotwordFileHasEntries(const std::string &path, bool *has_entries,
                           std::string *error);

// Loads one entry per line and serializes it as the comma-separated format used
// by FunASR Nano and Qwen3-ASR. A trailing transducer score (":3.5") is
// removed because prompt-based models do not support per-entry scores.
bool LoadPromptHotwordsCsv(const std::string &path, std::string *hotwords,
                           std::string *error);

// Validates the transducer tokenizer settings required by sherpa-onnx's
// EncodeHotwords(). Models advertising hotword support must declare the unit.
bool ValidateTransducerHotwordAssets(const ModelInfo &info, std::string *error);

} // namespace vinput::daemon::asr
