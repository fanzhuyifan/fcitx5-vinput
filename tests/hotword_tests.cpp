#include "common/asr/bpe_vocab_exporter.h"
#include "common/asr/model_manager.h"
#include "daemon/asr/hotword_utils.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>
#include <unistd.h>

namespace fs = std::filesystem;
using vinput::daemon::asr::IsPromptHotwordFamily;
using vinput::daemon::asr::IsTransducerHotwordFamily;
using vinput::daemon::asr::LoadPromptHotwordsCsv;
using vinput::daemon::asr::LoadTransducerHotwords;
using vinput::daemon::asr::ValidateTransducerHotwordAssets;

namespace {

void Require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "hotword test failed: " << message << '\n';
    std::exit(1);
  }
}

void WriteBytes(const fs::path &path, std::string_view bytes) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  Require(static_cast<bool>(output), "write test hotwords file");
}

void AppendVarint(std::string *data, std::uint64_t value) {
  while (value >= 0x80) {
    data->push_back(static_cast<char>((value & 0x7f) | 0x80));
    value >>= 7;
  }
  data->push_back(static_cast<char>(value));
}

void AppendPiece(std::string *model, std::string_view piece, float score) {
  std::string message;
  message.push_back(static_cast<char>(0x0a));
  AppendVarint(&message, piece.size());
  message.append(piece);
  message.push_back(static_cast<char>(0x15));
  const std::uint32_t bits = std::bit_cast<std::uint32_t>(score);
  for (int shift = 0; shift < 32; shift += 8) {
    message.push_back(static_cast<char>((bits >> shift) & 0xff));
  }

  model->push_back(static_cast<char>(0x0a));
  AppendVarint(model, message.size());
  model->append(message);
}

} // namespace

int main() {
  const fs::path root = fs::temp_directory_path() /
                        ("vinput-hotword-tests-" + std::to_string(getpid()));
  fs::create_directories(root);

  const fs::path sentencepiece_model = root / "bpe.model";
  const fs::path bpe_vocab = root / "bpe.vocab";
  std::string model_data;
  AppendPiece(&model_data, "<blk>", 0.0f);
  AppendPiece(&model_data, "▁hello", -1.25f);
  {
    std::ofstream output(sentencepiece_model, std::ios::binary);
    output.write(model_data.data(),
                 static_cast<std::streamsize>(model_data.size()));
  }

  std::string error;
  Require(ExportSentencePieceVocabulary(sentencepiece_model, bpe_vocab, &error),
          error);
  Require(ValidateBpeVocabulary(bpe_vocab, &error), error);

  const fs::path hotword_file = root / "hotwords.txt";
  {
    std::ofstream output(hotword_file);
    output << "语音识别:3.5\n"
           << "OpenAI\n"
           << "deep learning:2\n\n";
  }
  std::string prompt_hotwords;
  Require(LoadPromptHotwordsCsv(hotword_file, &prompt_hotwords, &error), error);
  Require(prompt_hotwords == "语音识别,OpenAI,deep learning",
          "prompt hotword conversion");

  std::string transducer_hotwords;
  Require(LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
          error);
  Require(transducer_hotwords ==
              "语音识别 :3.5\nOpenAI\ndeep learning :2\n",
          "transducer hotword conversion");

  {
    std::ofstream output(hotword_file);
    output << "invalid :score\n";
  }
  Require(!LoadPromptHotwordsCsv(hotword_file, &prompt_hotwords, &error),
          "invalid explicit scores must be rejected");

  {
    std::ofstream output(hotword_file);
    output << "https://example.com\n";
  }
  Require(LoadPromptHotwordsCsv(hotword_file, &prompt_hotwords, &error), error);
  Require(prompt_hotwords == "https://example.com",
          "non-numeric colon suffix remains literal text");

  {
    std::ofstream output(hotword_file);
    output << "spaced score :1.25\n";
  }
  Require(!LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
          "spaces before the score delimiter must be rejected");

  {
    std::ofstream output(hotword_file);
    output << "spaced score: 1.25\n";
  }
  Require(!LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
          "spaces after the score delimiter must be rejected");

  {
    std::ofstream output(hotword_file);
    output << "invalid :nan\n";
  }
  Require(!LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
          "non-finite explicit scores must be rejected");

  for (const std::string_view suffix :
       {"nan", "inf", "infinity", "-inf", "1e9999", "1.2junk", ""}) {
    WriteBytes(hotword_file, std::string("invalid:") + std::string(suffix) +
                                 "\n");
    Require(!LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
            "invalid attached scores must be rejected");
  }

  const std::vector<std::string> invalid_utf8 = {
      std::string("bad\0text\n", 9),
      std::string("bad\x01text\n", 9),
      std::string("bad\xc0\xaf\n", 6),
      std::string("bad\xe2\x28\xa1\n", 7),
      std::string("bad\xed\xa0\x80\n", 7),
      std::string("bad\xf4\x90\x80\x80\n", 8),
      std::string("bad\xf0", 4),
  };
  for (const auto &bytes : invalid_utf8) {
    WriteBytes(hotword_file, bytes);
    Require(!LoadPromptHotwordsCsv(hotword_file, &prompt_hotwords, &error),
            "invalid UTF-8 and control bytes must be rejected");
  }

  {
    std::ofstream output(hotword_file);
    output << " \n\t\n";
  }
  Require(LoadTransducerHotwords(hotword_file, &transducer_hotwords, &error),
          error);
  Require(transducer_hotwords.empty(), "empty transducer hotwords stay off");
  Require(LoadPromptHotwordsCsv(hotword_file, &prompt_hotwords, &error), error);
  Require(prompt_hotwords.empty(), "empty prompt hotwords stay off");

  Require(IsTransducerHotwordFamily("transducer"), "transducer family");
  Require(IsTransducerHotwordFamily("nemo_transducer"),
          "nemo transducer family");
  Require(!IsTransducerHotwordFamily("qwen3_asr"),
          "prompt family is not a transducer");
  Require(IsPromptHotwordFamily("funasr_nano"), "FunASR prompt family");
  Require(IsPromptHotwordFamily("qwen3_asr"), "Qwen3 prompt family");

  ModelInfo info;
  info.model_config = {{"modeling_unit", ""}};
  Require(!ValidateTransducerHotwordAssets(info, &error),
          "hotword-capable transducers require an explicit modeling unit");

  info.model_config = {{"modeling_unit", "cjkchar"}};
  Require(ValidateTransducerHotwordAssets(info, &error), error);

  info.files["bpe_vocab"] = bpe_vocab.string();
  for (const char *unit : {"bpe", "bbpe", "cjkchar+bpe"}) {
    info.model_config = {{"modeling_unit", unit}};
    Require(ValidateTransducerHotwordAssets(info, &error), error);
  }

  info.files.clear();
  info.model_config = {{"modeling_unit", "bpe"}};
  Require(!ValidateTransducerHotwordAssets(info, &error),
          "BPE units require a vocabulary");
  info.model_config = {{"modeling_unit", "unknown"}};
  Require(!ValidateTransducerHotwordAssets(info, &error),
          "unknown modeling units must be rejected");

  fs::remove_all(root);
  return 0;
}
