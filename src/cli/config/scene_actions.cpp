#include "cli/config/scene_actions.h"

#include <nlohmann/json.hpp>

#include "common/config/core_config.h"
#include "common/i18n.h"
#include "common/scene/postprocess_scene.h"
#include "common/utils/string_utils.h"

#include "cli/utils/cli_helpers.h"

int RunSceneConfigList(Formatter& fmt, const CliContext& ctx) {
  CoreConfig config = LoadCoreConfig();
  const auto& scenes = config.scenes.definitions;

  if (ctx.json_output) {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& scene : scenes) {
      bool active = (scene.id == config.scenes.activeScene);
      bool active_command = (scene.id == config.scenes.activeCommandScene);
      arr.push_back({{"id", scene.id},
                     {"label", vinput::scene::DisplayLabel(scene)},
                     {"prompt", scene.prompt},
                     {"provider_id", scene.provider_id},
                     {"model", scene.model},
                     {"candidate_count", scene.candidate_count},
                     {"timeout_ms", scene.timeout_ms},
                     {"context_lines", scene.context_lines},
                     {"builtin", scene.builtin},
                     {"active", active},
                     {"active_dictation", active},
                     {"active_command", active_command}});
    }
    fmt.PrintJson(arr);
    return 0;
  }

  std::vector<std::string> headers = {_("ID"),         _("LABEL"),     _("PROVIDER"), _("MODEL"),
                                      _("CANDIDATES"), _("DICTATION"), _("COMMAND")};
  std::vector<std::vector<std::string>> rows;
  for (const auto& scene : scenes) {
    std::string label = vinput::scene::DisplayLabel(scene);
    std::string active = (scene.id == config.scenes.activeScene) ? "[*]" : "[ ]";
    std::string active_command = (scene.id == config.scenes.activeCommandScene) ? "[*]" : "[ ]";
    std::string provider = scene.provider_id.empty() ? "-" : scene.provider_id;
    std::string model = scene.model.empty() ? "-" : scene.model;
    rows.push_back({scene.id, label, provider, model, std::to_string(scene.candidate_count), active,
                    active_command});
  }
  fmt.PrintTable(headers, rows);
  return 0;
}

int RunSceneConfigAdd(const std::string& id, const std::string& label, const std::string& prompt,
                      const std::string& provider_id, const std::string& model, int candidate_count,
                      int timeout_ms, int context_lines, Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();

  vinput::scene::Definition def;
  def.id = vinput::str::TrimAsciiWhitespace(id);
  def.label = vinput::str::TrimAsciiWhitespace(label);
  def.prompt = vinput::str::TrimAsciiWhitespace(prompt);
  def.provider_id = vinput::str::TrimAsciiWhitespace(provider_id);
  def.model = vinput::str::TrimAsciiWhitespace(model);
  def.candidate_count = candidate_count;
  def.timeout_ms = timeout_ms;
  def.context_lines = context_lines;

  vinput::scene::Config scene_config = ToSceneConfig(config.scenes);
  std::string error;
  if (!vinput::scene::AddScene(&scene_config, def, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  FromSceneConfig(config.scenes, scene_config);

  if (!SaveConfigOrFail(config, fmt))
    return 1;

  fmt.PrintSuccess(vinput::str::FmtStr(_("Scene '%s' added."), id));
  return 0;
}

int RunSceneConfigUseDictation(const std::string& id, Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();

  vinput::scene::Config scene_config = ToSceneConfig(config.scenes);
  std::string error;
  if (!vinput::scene::SetActiveScene(&scene_config, id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  FromSceneConfig(config.scenes, scene_config);

  if (!SaveConfigOrFail(config, fmt))
    return 1;

  fmt.PrintSuccess(vinput::str::FmtStr(_("Dictation scene set to '%s'."), id));
  return 0;
}

int RunSceneConfigUseCommand(const std::string& id, Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();

  vinput::scene::Config scene_config = ToSceneConfig(config.scenes);
  std::string error;
  if (!vinput::scene::SetActiveScene(&scene_config, id, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  config.scenes.activeCommandScene = scene_config.activeSceneId;

  if (!SaveConfigOrFail(config, fmt))
    return 1;

  fmt.PrintSuccess(vinput::str::FmtStr(_("Command scene set to '%s'."), id));
  return 0;
}

int RunSceneConfigRemove(const std::string& id, bool force, Formatter& fmt, const CliContext& ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();

  if (!force && id == config.scenes.activeCommandScene) {
    fmt.PrintError(vinput::str::FmtStr(_("Cannot remove active command scene '%s'."), id));
    return 1;
  }

  vinput::scene::Config scene_config = ToSceneConfig(config.scenes);
  std::string error;
  if (!vinput::scene::RemoveScene(&scene_config, id, force, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  FromSceneConfig(config.scenes, scene_config);

  if (!SaveConfigOrFail(config, fmt))
    return 1;

  fmt.PrintSuccess(vinput::str::FmtStr(_("Scene '%s' removed."), id));
  return 0;
}

int RunSceneConfigEdit(const std::string& id, const std::string& label, const std::string& prompt,
                       const std::string& provider_id, const std::string& model,
                       int candidate_count, int timeout_ms, int context_lines, bool hasLabel,
                       bool hasPrompt, bool hasProvider, bool hasModel, bool hasCandidates,
                       bool hasTimeout, bool hasContextLines, Formatter& fmt,
                       const CliContext& ctx) {
  (void)ctx;
  CoreConfig config = LoadCoreConfig();

  const vinput::scene::Definition* existing = nullptr;
  for (const auto& scene : config.scenes.definitions) {
    if (scene.id == id) {
      existing = &scene;
      break;
    }
  }
  if (!existing) {
    fmt.PrintError(vinput::str::FmtStr(_("Scene '%s' not found."), id));
    return 1;
  }

  vinput::scene::Definition updated = *existing;
  if (hasLabel)
    updated.label = vinput::str::TrimAsciiWhitespace(label);
  if (hasPrompt)
    updated.prompt = vinput::str::TrimAsciiWhitespace(prompt);
  if (hasProvider)
    updated.provider_id = vinput::str::TrimAsciiWhitespace(provider_id);
  if (hasModel)
    updated.model = vinput::str::TrimAsciiWhitespace(model);
  if (hasCandidates)
    updated.candidate_count = candidate_count;
  if (hasTimeout)
    updated.timeout_ms = timeout_ms;
  if (hasContextLines)
    updated.context_lines = context_lines;

  vinput::scene::Config scene_config = ToSceneConfig(config.scenes);
  std::string error;
  if (!vinput::scene::UpdateScene(&scene_config, id, updated, &error)) {
    fmt.PrintError(error);
    return 1;
  }
  FromSceneConfig(config.scenes, scene_config);

  if (!SaveConfigOrFail(config, fmt))
    return 1;

  fmt.PrintSuccess(vinput::str::FmtStr(_("Scene '%s' updated."), id));
  return 0;
}
