# fish completion for vinput

function __fish_vinput_installed_models
    vinput model list -j 2>/dev/null | string match -r '"id":\s*"([^"]+)"' | string replace -r '"id":\s*"([^"]+)"' '$1'
end

function __fish_vinput_installed_scenes
    vinput scene list -j 2>/dev/null | string match -r '"id":\s*"([^"]+)"' | string replace -r '"id":\s*"([^"]+)"' '$1'
end

# Global options
complete -c vinput -s j -l json -d "Output in JSON format"
complete -c vinput -s h -l help -d "Print help message and exit"
complete -c vinput -s v -l version -d "Display program version and exit"

# Top-level commands
complete -c vinput -n "__fish_use_subcommand" -a init -d "Initialize default config and directories"
complete -c vinput -n "__fish_use_subcommand" -a model -d "Manage offline local ASR models"
complete -c vinput -n "__fish_use_subcommand" -a provider -d "Manage cloud ASR providers"
complete -c vinput -n "__fish_use_subcommand" -a llm -d "Manage LLM providers"
complete -c vinput -n "__fish_use_subcommand" -a adapter -d "Manage LLM adapters"
complete -c vinput -n "__fish_use_subcommand" -a hotword -d "Manage hotword file"
complete -c vinput -n "__fish_use_subcommand" -a device -d "Manage audio capture devices"
complete -c vinput -n "__fish_use_subcommand" -a scene -d "Manage recognition scenes"
complete -c vinput -n "__fish_use_subcommand" -a config -d "Read or write configuration values"
complete -c vinput -n "__fish_use_subcommand" -a daemon -d "Control daemon lifecycle"
complete -c vinput -n "__fish_use_subcommand" -a recording -d "Control voice recording"
complete -c vinput -n "__fish_use_subcommand" -a rec -d "Control voice recording (alias)"

# init
complete -c vinput -n "__fish_seen_subcommand_from init" -s f -l force -d "Overwrite existing config"

# model
complete -c vinput -n "__fish_seen_subcommand_from model" -a "list ls" -d "List installed or remote models"
complete -c vinput -n "__fish_seen_subcommand_from model" -a add -d "Download and install model"
complete -c vinput -n "__fish_seen_subcommand_from model" -a use -d "Set active local model"
complete -c vinput -n "__fish_seen_subcommand_from model" -a "remove rm" -d "Uninstall local model"
complete -c vinput -n "__fish_seen_subcommand_from model" -a info -d "Show model details"
complete -c vinput -n "__fish_seen_subcommand_from model; and __fish_seen_subcommand_from list ls" -s a -l available -d "List remote models"
complete -c vinput -n "__fish_seen_subcommand_from model; and __fish_seen_subcommand_from use remove rm info" -a "(__fish_vinput_installed_models)"

# provider
complete -c vinput -n "__fish_seen_subcommand_from provider" -a "list ls" -d "List configured or remote providers"
complete -c vinput -n "__fish_seen_subcommand_from provider" -a add -d "Install provider from registry"
complete -c vinput -n "__fish_seen_subcommand_from provider" -a use -d "Set active ASR provider"
complete -c vinput -n "__fish_seen_subcommand_from provider" -a "edit e" -d "Edit provider script in editor"
complete -c vinput -n "__fish_seen_subcommand_from provider" -a "remove rm" -d "Uninstall ASR provider"
complete -c vinput -n "__fish_seen_subcommand_from provider; and __fish_seen_subcommand_from list ls" -s a -l available -d "List remote providers"

# llm
complete -c vinput -n "__fish_seen_subcommand_from llm" -a "list ls" -d "List configured LLM providers"
complete -c vinput -n "__fish_seen_subcommand_from llm" -a add -d "Add an LLM provider"
complete -c vinput -n "__fish_seen_subcommand_from llm" -a "edit e" -d "Edit an LLM provider"
complete -c vinput -n "__fish_seen_subcommand_from llm" -a "remove rm" -d "Remove an LLM provider"
complete -c vinput -n "__fish_seen_subcommand_from llm" -a test -d "Test LLM provider connectivity"
complete -c vinput -n "__fish_seen_subcommand_from llm; and __fish_seen_subcommand_from add edit e" -s u -l base-url -d "Base URL"
complete -c vinput -n "__fish_seen_subcommand_from llm; and __fish_seen_subcommand_from add edit e" -s k -l api-key -d "API key"
complete -c vinput -n "__fish_seen_subcommand_from llm; and __fish_seen_subcommand_from add edit e" -s e -l extra-body -d "Extra JSON body"

# adapter
complete -c vinput -n "__fish_seen_subcommand_from adapter" -a "list ls" -d "List local or remote adapters"
complete -c vinput -n "__fish_seen_subcommand_from adapter" -a add -d "Install an adapter"
complete -c vinput -n "__fish_seen_subcommand_from adapter" -a start -d "Start an adapter process"
complete -c vinput -n "__fish_seen_subcommand_from adapter" -a stop -d "Stop an adapter process"
complete -c vinput -n "__fish_seen_subcommand_from adapter; and __fish_seen_subcommand_from list ls" -s a -l available -d "List remote adapters"

# hotword
complete -c vinput -n "__fish_seen_subcommand_from hotword" -a get -d "Show configured hotword path"
complete -c vinput -n "__fish_seen_subcommand_from hotword" -a set -d "Set hotword file path"
complete -c vinput -n "__fish_seen_subcommand_from hotword" -a clear -d "Clear hotword file path"
complete -c vinput -n "__fish_seen_subcommand_from hotword" -a "edit e" -d "Edit hotword file in editor"

# device
complete -c vinput -n "__fish_seen_subcommand_from device" -a "list ls" -d "List available audio input devices"
complete -c vinput -n "__fish_seen_subcommand_from device" -a use -d "Set active capture device"

# scene
complete -c vinput -n "__fish_seen_subcommand_from scene" -a "list ls" -d "List all scenes"
complete -c vinput -n "__fish_seen_subcommand_from scene" -a add -d "Add a new scene"
complete -c vinput -n "__fish_seen_subcommand_from scene" -a "edit e" -d "Edit a scene"
complete -c vinput -n "__fish_seen_subcommand_from scene" -a use -d "Set active scene"
complete -c vinput -n "__fish_seen_subcommand_from scene" -a "remove rm" -d "Remove a scene"
complete -c vinput -n "__fish_seen_subcommand_from scene; and __fish_seen_subcommand_from use remove rm" -a "(__fish_vinput_installed_scenes)"

# config
complete -c vinput -n "__fish_seen_subcommand_from config" -a get -d "Get a config value by JSON Pointer"
complete -c vinput -n "__fish_seen_subcommand_from config" -a set -d "Set a config value by JSON Pointer"
complete -c vinput -n "__fish_seen_subcommand_from config" -a "edit e" -d "Open config in editor"
complete -c vinput -n "__fish_seen_subcommand_from config; and __fish_seen_subcommand_from edit e" -a "core fcitx"
complete -c vinput -n "__fish_seen_subcommand_from config; and __fish_seen_subcommand_from set" -s i -l stdin -d "Read value from stdin"

# daemon
complete -c vinput -n "__fish_seen_subcommand_from daemon" -a status -d "Show daemon status"
complete -c vinput -n "__fish_seen_subcommand_from daemon" -a start -d "Start daemon"
complete -c vinput -n "__fish_seen_subcommand_from daemon" -a stop -d "Stop daemon"
complete -c vinput -n "__fish_seen_subcommand_from daemon" -a restart -d "Restart daemon"
complete -c vinput -n "__fish_seen_subcommand_from daemon" -a log -d "Show daemon logs"
complete -c vinput -n "__fish_seen_subcommand_from daemon; and __fish_seen_subcommand_from log" -s f -l follow -d "Follow log output"
complete -c vinput -n "__fish_seen_subcommand_from daemon; and __fish_seen_subcommand_from log" -s n -l lines -d "Number of log lines"

# recording
complete -c vinput -n "__fish_seen_subcommand_from recording rec" -a start -d "Start recording"
complete -c vinput -n "__fish_seen_subcommand_from recording rec" -a stop -d "Stop recording and recognize"
complete -c vinput -n "__fish_seen_subcommand_from recording rec" -a toggle -d "Toggle recording"
complete -c vinput -n "__fish_seen_subcommand_from recording rec; and __fish_seen_subcommand_from stop toggle" -s s -l scene -a "(__fish_vinput_installed_scenes)"
