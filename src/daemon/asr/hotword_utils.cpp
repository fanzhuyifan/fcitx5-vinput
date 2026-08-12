#include "daemon/asr/hotword_utils.h"

#include "common/asr/bpe_vocab_exporter.h"
#include "common/asr/model_manager.h"
#include "daemon/asr/sherpa_json_helpers.h"

#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <locale>
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

struct HotwordEntry {
  std::string text;
  std::string score;
};

bool IsValidScore(std::string_view value) {
  if (value.empty()) {
    return false;
  }
  std::istringstream input{std::string(value)};
  input.imbue(std::locale::classic());
  input >> std::noskipws;
  float score = 0.0f;
  input >> score;
  return input.eof() && !input.fail() && std::isfinite(score);
}

bool ParseHotwordEntry(std::string line, HotwordEntry *entry,
                       std::string *error) {
  line = TrimAsciiWhitespace(std::move(line));
  if (line.empty()) {
    entry->text.clear();
    entry->score.clear();
    return true;
  }

  const auto colon = line.rfind(':');
  if (colon == std::string::npos) {
    entry->text = std::move(line);
    entry->score.clear();
    return true;
  }

  const std::string raw_score = line.substr(colon + 1);
  const std::string score = TrimAsciiWhitespace(raw_score);
  const std::string text = TrimAsciiWhitespace(line.substr(0, colon));
  if (IsValidScore(score)) {
    const bool whitespace_before =
        colon > 0 &&
        std::isspace(static_cast<unsigned char>(line[colon - 1]));
    const bool whitespace_after = raw_score != score;
    if (whitespace_before || whitespace_after) {
      if (error) {
        *error = "invalid hotword score; use 'word:3.5' without spaces: " +
                 line;
      }
      return false;
    }
    if (text.empty()) {
      if (error) {
        *error = "hotword score has no preceding word or phrase: " + line;
      }
      return false;
    }
    entry->text = text;
    entry->score = score;
    return true;
  }

  // A separated ':...' token is an invalid score. A non-numeric suffix
  // attached directly to a term remains literal text.
  if (colon > 0 &&
      std::isspace(static_cast<unsigned char>(line[colon - 1]))) {
    if (error) {
      *error = "invalid hotword score; use 'word:3.5' without spaces: " + line;
    }
    return false;
  }

  entry->text = std::move(line);
  entry->score.clear();
  return true;
}

bool LoadHotwordEntries(const std::string &path,
                        std::vector<HotwordEntry> *entries,
                        std::string *error) {
  std::string content;
  if (!ReadHotwordFile(path, &content, error)) {
    return false;
  }

  entries->clear();
  std::istringstream lines(content);
  std::string line;
  while (std::getline(lines, line)) {
    HotwordEntry entry;
    if (!ParseHotwordEntry(std::move(line), &entry, error)) {
      return false;
    }
    if (!entry.text.empty()) {
      entries->push_back(std::move(entry));
    }
  }
  if (error) {
    error->clear();
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

bool LoadTransducerHotwords(const std::string &path, std::string *hotwords,
                            std::string *error) {
  std::vector<HotwordEntry> entries;
  if (!LoadHotwordEntries(path, &entries, error)) {
    return false;
  }

  hotwords->clear();
  for (const auto &entry : entries) {
    *hotwords += entry.text;
    if (!entry.score.empty()) {
      // sherpa-onnx expects the score as a separate trailing token.
      *hotwords += " :" + entry.score;
    }
    hotwords->push_back('\n');
  }
  return true;
}

bool LoadPromptHotwordsCsv(const std::string &path, std::string *hotwords,
                           std::string *error) {
  std::vector<HotwordEntry> entries;
  if (!LoadHotwordEntries(path, &entries, error)) {
    return false;
  }

  hotwords->clear();
  for (std::size_t i = 0; i < entries.size(); ++i) {
    if (i) {
      hotwords->push_back(',');
    }
    *hotwords += entries[i].text;
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
