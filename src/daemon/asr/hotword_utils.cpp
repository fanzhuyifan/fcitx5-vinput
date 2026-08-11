#include "daemon/asr/hotword_utils.h"

#include "common/asr/bpe_vocab_exporter.h"
#include "common/asr/model_manager.h"
#include "daemon/asr/sherpa_json_helpers.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

namespace vinput::daemon::asr {

namespace {

constexpr std::uintmax_t kMaxHotwordFileBytes = 1024 * 1024;

std::string TrimAsciiWhitespace(std::string value) {
  const auto first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = value.find_last_not_of(" \t\r\n");
  return value.substr(first, last - first + 1);
}

bool ReadHotwordFile(const std::string &path, std::string *content,
                     std::string *error) {
  if (path.empty()) {
    if (content) {
      content->clear();
    }
    if (error) {
      error->clear();
    }
    return true;
  }

  std::error_code ec;
  const std::filesystem::path file_path(path);
  if (!std::filesystem::is_regular_file(file_path, ec) || ec) {
    if (error) {
      *error = "hotwords file is not a regular file: " + path;
    }
    return false;
  }
  const auto size = std::filesystem::file_size(file_path, ec);
  if (ec) {
    if (error) {
      *error =
          "failed to inspect hotwords file '" + path + "': " + ec.message();
    }
    return false;
  }
  if (size > kMaxHotwordFileBytes) {
    if (error) {
      *error = "hotwords file exceeds 1 MiB limit: " + path;
    }
    return false;
  }

  std::ifstream input(file_path, std::ios::binary);
  if (!input) {
    if (error) {
      *error = "failed to open hotwords file: " + path;
    }
    return false;
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  if (!input.good() && !input.eof()) {
    if (error) {
      *error = "failed to read hotwords file: " + path;
    }
    return false;
  }

  if (content) {
    *content = std::move(buffer).str();
  }
  if (error) {
    error->clear();
  }
  return true;
}

bool IsValidScore(std::string_view token) {
  if (token.size() < 2 || token.front() != ':') {
    return false;
  }
  const std::string number(token.substr(1));
  char *end = nullptr;
  const float value = std::strtof(number.c_str(), &end);
  return end == number.c_str() + number.size() && std::isfinite(value);
}

bool NormalizePromptEntry(const std::string &line, std::string *entry,
                          std::string *error) {
  std::istringstream words(line);
  std::vector<std::string> tokens;
  std::string token;
  while (words >> token) {
    tokens.push_back(token);
  }
  if (tokens.empty()) {
    entry->clear();
    return true;
  }

  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (tokens[i].empty() || tokens[i].front() != ':') {
      continue;
    }
    if (i + 1 != tokens.size() || !IsValidScore(tokens[i])) {
      if (error) {
        *error =
            "invalid hotword score; use a trailing score such as ':3.5': " +
            line;
      }
      return false;
    }
    tokens.pop_back();
    break;
  }

  if (tokens.empty()) {
    if (error) {
      *error = "hotword score has no preceding word or phrase: " + line;
    }
    return false;
  }

  entry->clear();
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i) {
      entry->push_back(' ');
    }
    *entry += tokens[i];
  }
  return true;
}

} // namespace

bool IsTransducerHotwordFamily(std::string_view family) {
  return family == "transducer" || family == "nemo_transducer";
}

bool IsPromptHotwordFamily(std::string_view family) {
  return family == "funasr_nano" || family == "qwen3_asr";
}

bool HotwordFileHasEntries(const std::string &path, bool *has_entries,
                           std::string *error) {
  std::string content;
  if (!ReadHotwordFile(path, &content, error)) {
    return false;
  }
  if (has_entries) {
    *has_entries = !TrimAsciiWhitespace(std::move(content)).empty();
  }
  return true;
}

bool LoadPromptHotwordsCsv(const std::string &path, std::string *hotwords,
                           std::string *error) {
  std::string content;
  if (!ReadHotwordFile(path, &content, error)) {
    return false;
  }

  std::istringstream lines(content);
  std::string line;
  std::vector<std::string> entries;
  while (std::getline(lines, line)) {
    line = TrimAsciiWhitespace(std::move(line));
    if (line.empty()) {
      continue;
    }
    std::string entry;
    if (!NormalizePromptEntry(line, &entry, error)) {
      return false;
    }
    entries.push_back(std::move(entry));
  }

  hotwords->clear();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i) {
      hotwords->push_back(',');
    }
    *hotwords += entries[i];
  }
  if (error) {
    error->clear();
  }
  return true;
}

bool ValidateTransducerHotwordAssets(const ModelInfo &info,
                                     std::string *error) {
  const std::string modeling_unit =
      JsonString(info.model_config, "modeling_unit");
  if (modeling_unit.empty()) {
    if (error) {
      *error = "hotword-capable transducer is missing model.modeling_unit; "
               "reinstall or update the model";
    }
    return false;
  }
  if (modeling_unit != "cjkchar" && modeling_unit != "bpe" &&
      modeling_unit != "bbpe" && modeling_unit != "cjkchar+bpe") {
    if (error) {
      *error = "unsupported sherpa-onnx hotword modeling_unit '" +
               modeling_unit + "'";
    }
    return false;
  }

  if (modeling_unit == "bpe" || modeling_unit == "bbpe" ||
      modeling_unit == "cjkchar+bpe") {
    const std::string bpe_vocab = info.File("bpe_vocab");
    if (bpe_vocab.empty() || !std::filesystem::is_regular_file(bpe_vocab)) {
      if (error) {
        *error =
            "modeling_unit '" + modeling_unit + "' requires model.bpe_vocab";
      }
      return false;
    }
    if (!ValidateBpeVocabulary(bpe_vocab, error)) {
      return false;
    }
  }

  if (error) {
    error->clear();
  }
  return true;
}

} // namespace vinput::daemon::asr
