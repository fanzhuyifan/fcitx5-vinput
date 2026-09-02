#include <cstdlib>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>

#include "common/config/core_config.h"

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

} // namespace

int main() {
  TestMissingFieldMigration();
  TestJsonRoundTrip();
  TestInvalidSelectionNormalization();
  TestValidation();
  TestResolverSelectionAndFallback();
  return 0;
}
