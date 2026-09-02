#pragma once

#include <fcitx-utils/key.h>
#include <optional>

enum class SceneMenuTarget { Dictation, Command };

inline std::optional<SceneMenuTarget>
MatchSceneMenuShortcut(const fcitx::Key& key, const fcitx::KeyList& dictation_keys,
                       const fcitx::KeyList& command_keys,
                       std::optional<SceneMenuTarget> current_target = std::nullopt) {
  const bool matches_dictation = key.checkKeyList(dictation_keys);
  const bool matches_command = key.checkKeyList(command_keys);
  if (matches_dictation && matches_command) {
    if (current_target) {
      return *current_target == SceneMenuTarget::Dictation ? SceneMenuTarget::Command
                                                           : SceneMenuTarget::Dictation;
    }
    return SceneMenuTarget::Dictation;
  }
  if (matches_dictation) {
    return SceneMenuTarget::Dictation;
  }
  if (matches_command) {
    return SceneMenuTarget::Command;
  }
  return std::nullopt;
}
