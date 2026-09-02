#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>

#include "common/dbus/dbus_interface.h"

#include "daemon/postprocess/post_processor.h"
#include "daemon/runtime/recognition_pipeline.h"

namespace {

using vinput::daemon::runtime::RecognitionOrder;
using vinput::daemon::runtime::RecognitionPipeline;

void Require(bool condition, std::string_view message) {
  if (!condition) {
    std::cerr << "recognition pipeline test failed: " << message << '\n';
    std::exit(1);
  }
}

vinput::scene::Definition MakeScene(std::string id, int candidate_count,
                                    std::string provider_id = {}) {
  vinput::scene::Definition scene;
  scene.id = std::move(id);
  scene.prompt = "{{selected}} {{asr}}";
  scene.provider_id = std::move(provider_id);
  if (!scene.provider_id.empty()) {
    scene.model = "test-model";
  }
  scene.candidate_count = candidate_count;
  return scene;
}

CoreConfig MakeConfig() {
  CoreConfig config;
  config.scenes.activeScene = std::string(vinput::scene::kRawSceneId);
  config.scenes.activeCommandScene = "command.active";
  config.scenes.definitions = {
      MakeScene(std::string(vinput::scene::kRawSceneId), 0),
      MakeScene(std::string(vinput::scene::kCommandSceneId), 0),
      MakeScene("command.active", 1, "unconfigured-provider"),
      MakeScene("command.routed", 0),
  };
  return config;
}

bool EntersPostprocessing(RecognitionPipeline* pipeline, const CoreConfig& config,
                          std::string scene_id) {
  RecognitionOrder order;
  order.recognized_text = "rewrite this";
  order.scene_id = std::move(scene_id);
  order.is_command = true;
  order.selected_text = "source text";

  bool entered = false;
  const auto result = pipeline->Process(order, config, [&]() { entered = true; });
  Require(!result.payload.candidates.empty(), "command processing returns a fallback candidate");
  return entered;
}

void TestCommandSceneRouting() {
  PostProcessor post_processor;
  RecognitionPipeline pipeline(&post_processor);
  const CoreConfig config = MakeConfig();

  const std::string route_prefix(vinput::dbus::kCommandSceneRoutePrefix);
  Require(!EntersPostprocessing(&pipeline, config, route_prefix + "command.routed"),
          "a tagged command route selects the requested scene");
  Require(EntersPostprocessing(&pipeline, config, "command.routed"),
          "an untagged legacy route uses the configured active command scene");
  Require(EntersPostprocessing(&pipeline, config, route_prefix + "missing"),
          "a stale tagged route falls back to the configured active command scene");
}

} // namespace

int main() {
  TestCommandSceneRouting();
  return 0;
}
