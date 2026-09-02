#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "common/config/core_config.h"
#include "common/config/vinput_config.h"

#include "addon/input/scene_menu_shortcut.h"

namespace {

using json = nlohmann::ordered_json;

void Require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "core config test failed: " << message << '\n';
    std::exit(1);
  }
}

vinput::scene::Definition MakeScene(std::string id) {
  vinput::scene::Definition scene;
  scene.id = std::move(id);
  return scene;
}

CoreConfig MakeValidConfig() {
  CoreConfig config;
  auto raw = MakeScene(std::string(vinput::scene::kRawSceneId));
  raw.candidate_count = 0;
  config.scenes.activeScene = raw.id;
  config.scenes.definitions = {
      std::move(raw),
      MakeScene(std::string(vinput::scene::kCommandSceneId)),
      MakeScene("command.custom"),
  };
  return config;
}

void TestMissingFieldMigration() {
  json legacy = json::object();
  legacy["version"] = 1;
  legacy["scenes"] = {
      {"active_scene", vinput::scene::kRawSceneId},
      {"definitions", json::array({
                          {{"id", vinput::scene::kRawSceneId}, {"candidate_count", 0}},
                          {{"id", vinput::scene::kCommandSceneId}},
                      })},
  };

  CoreConfig config;
  from_json(legacy, config);
  Require(config.scenes.activeCommandScene == vinput::scene::kCommandSceneId,
          "legacy JSON defaults active_command_scene to __command__");

  NormalizeCoreConfig(&config);
  Require(config.scenes.activeCommandScene == vinput::scene::kCommandSceneId,
          "legacy JSON keeps a valid command scene after normalization");
  std::string error;
  Require(ValidateCoreConfig(config, &error), error);
}

void TestJsonRoundTrip() {
  CoreConfig config = MakeValidConfig();
  config.scenes.activeCommandScene = "command.custom";

  json serialized;
  to_json(serialized, config);
  Require(serialized.at("scenes").at("active_command_scene") == "command.custom",
          "active command scene is serialized");

  CoreConfig parsed;
  from_json(serialized, parsed);
  Require(parsed.scenes.activeCommandScene == "command.custom",
          "active command scene survives a JSON round trip");
}

void TestInvalidSelectionNormalization() {
  for (const std::string& invalid : {std::string{}, std::string{"missing"}}) {
    CoreConfig config = MakeValidConfig();
    config.scenes.activeCommandScene = invalid;

    NormalizeCoreConfig(&config);
    Require(config.scenes.activeCommandScene == vinput::scene::kCommandSceneId,
            "invalid active command scenes normalize to __command__");
  }
}

void TestValidation() {
  CoreConfig config = MakeValidConfig();
  config.scenes.activeCommandScene = "command.custom";
  std::string error;
  Require(ValidateCoreConfig(config, &error), error);

  config.scenes.activeCommandScene = "missing";
  Require(!ValidateCoreConfig(config, &error), "validation rejects a missing active command scene");
  Require(error.find("Active command scene 'missing' does not exist.") != std::string::npos,
          "validation reports the invalid active command scene");
}

void TestResolverSelectionAndFallback() {
  CoreConfig config = MakeValidConfig();
  config.scenes.activeCommandScene = "command.custom";

  const auto* selected = ResolveActiveCommandScene(config);
  Require(selected != nullptr && selected->id == "command.custom",
          "resolver selects the configured active command scene");

  const auto* builtin = FindCommandScene(config);
  Require(builtin != nullptr && builtin->id == vinput::scene::kCommandSceneId,
          "FindCommandScene remains a builtin-only lookup");
  Require(selected != builtin, "custom command selection does not change builtin lookup");

  config.scenes.activeCommandScene = "missing";
  Require(ResolveActiveCommandScene(config) == builtin,
          "resolver falls back to the builtin command scene");
}

void TestSceneMenuShortcutConfig() {
  VinputConfig defaults;
  Require(defaults.sceneMenuKeys.value() == fcitx::KeyList{fcitx::Key(FcitxKey_Shift_R)},
          "dictation scene menu defaults to Right Shift");
  Require(defaults.commandSceneMenuKeys.value() == fcitx::KeyList{fcitx::Key(FcitxKey_F9)},
          "command scene menu defaults to F9");

  fcitx::RawConfig legacy;
  legacy["SceneMenuKey/0"] = "F9";
  VinputConfig migrated;
  migrated.load(legacy, true);
  Require(migrated.sceneMenuKeys.value() == fcitx::KeyList{fcitx::Key(FcitxKey_F9)},
          "a legacy F9 dictation binding is preserved");
  Require(migrated.commandSceneMenuKeys.value() == fcitx::KeyList{fcitx::Key(FcitxKey_F9)},
          "a missing command scene binding receives its default");

  fcitx::RawConfig custom;
  custom["CommandSceneMenuKey/0"] = "F10";
  custom["CommandSceneMenuKey/1"] = "Control+F9";
  VinputConfig configured;
  configured.load(custom, true);
  const fcitx::KeyList expected{
      fcitx::Key(FcitxKey_F10),
      fcitx::Key(FcitxKey_F9, fcitx::KeyState::Ctrl),
  };
  Require(configured.commandSceneMenuKeys.value() == expected,
          "custom command scene menu keys load from RawConfig");

  fcitx::RawConfig saved;
  configured.save(saved);
  const auto* first = saved.valueByPath("CommandSceneMenuKey/0");
  const auto* second = saved.valueByPath("CommandSceneMenuKey/1");
  Require(first && *first == "F10", "the first command scene menu key is serialized");
  Require(second && *second == "Control+F9", "the second command scene menu key is serialized");
}

void TestSceneMenuShortcutPrecedence() {
  const fcitx::Key key(FcitxKey_F9);
  const fcitx::KeyList dictation{key};
  const fcitx::KeyList command{key};
  const auto overlap = MatchSceneMenuShortcut(key, dictation, command);
  Require(overlap && *overlap == SceneMenuTarget::Dictation,
          "an overlapping shortcut preserves dictation menu precedence");
  const auto from_dictation =
      MatchSceneMenuShortcut(key, dictation, command, SceneMenuTarget::Dictation);
  Require(from_dictation && *from_dictation == SceneMenuTarget::Command,
          "an overlapping shortcut switches from the dictation menu to the command menu");
  const auto from_command =
      MatchSceneMenuShortcut(key, dictation, command, SceneMenuTarget::Command);
  Require(from_command && *from_command == SceneMenuTarget::Dictation,
          "an overlapping shortcut switches from the command menu to the dictation menu");

  const auto command_only = MatchSceneMenuShortcut(key, {fcitx::Key(FcitxKey_Shift_R)}, command);
  Require(command_only && *command_only == SceneMenuTarget::Command,
          "the command shortcut selects the command menu");
  Require(!MatchSceneMenuShortcut(fcitx::Key(FcitxKey_F10), dictation, command),
          "an unrelated key does not select a scene menu");
}

} // namespace

int main() {
  TestMissingFieldMigration();
  TestJsonRoundTrip();
  TestInvalidSelectionNormalization();
  TestValidation();
  TestResolverSelectionAndFallback();
  TestSceneMenuShortcutConfig();
  TestSceneMenuShortcutPrecedence();
  return 0;
}
