#pragma once

#include <filesystem>
#include <string>

// Exports the piece table embedded in a SentencePiece ModelProto to the text
// vocabulary format expected by sherpa-onnx's model.bpe_vocab option.
bool ExportSentencePieceVocabulary(const std::filesystem::path& model_path,
                                   const std::filesystem::path& vocab_path, std::string* error);

// Checks the tab-separated piece/score format consumed by simple-sentencepiece.
bool ValidateBpeVocabulary(const std::filesystem::path& vocab_path, std::string* error);
