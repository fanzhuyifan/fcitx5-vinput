---
title: Scenes & LLM
description: Rewrite, polish, and translate recognition results with LLM.
---

## Concepts

Raw text from ASR may contain filler words, typos, or formatting issues. **Scenes** define a prompt that tells an **LLM** how to rewrite the text.

```
ASR raw text → [Scene prompt + LLM] → Rewritten text
```

Core relationships:

- A **scene** is a configuration: prompt (instruction) + bound LLM provider + model
- An **LLM provider** is an OpenAI-compatible API endpoint (e.g. Ollama, Groq, OpenAI)
- An **LLM adapter** is an optional local bridge process that wraps a non-standard local model into an OpenAI-compatible API for a provider to point to

In short: **the scene decides "how to rewrite", the LLM provider decides "who rewrites", the adapter solves "incompatible API"**.

When LLM rewriting is not needed, use the built-in `__raw__` scene — recognition results go directly to input.

## Scenes

### Concept

Each scene contains:

| Field | Description |
|-------|-------------|
| `id` | Unique identifier |
| `label` | Display name in the menu |
| `prompt` | System prompt sent to the LLM |
| `provider_id` | Bound LLM provider |
| `model` | Model name to use |
| `context_lines` | Number of previous text lines sent as context to the LLM |
| `candidate_count` | Number of candidates to return |

LLM is only invoked when `provider_id` + `model` + `prompt` are all configured.

Built-in scenes:
- **`__raw__`** — Bypasses LLM, outputs raw recognition text
- **`__command__`** — Used by command mode (see below)

Switch scenes at runtime with `Shift_R`.

Corresponding config:

```json
{
  "scenes": {
    "active_scene": "__raw__",
    "active_command_scene": "__command__",
    "definitions": [
      {
        "id": "__raw__",
        "candidate_count": 0
      },
      {
        "id": "default",
        "label": "Default",
        "prompt": "Correct ASR errors, keep original meaning...",
        "provider_id": "groq",
        "model": "openai/gpt-oss-120b",
        "context_lines": 3,
        "candidate_count": 5
      }
    ]
  }
}
```

### GUI

In Vinput GUI, manage scenes in the **LLM** tab: add, edit prompt, bind provider and model, and configure **Context Lines**. Select a scene and click **Use for Dictation** or **Use for Command** to choose the two active scenes independently. The list marks them with **[Dictation]** and **[Command]**; one scene may serve both roles.

`context_lines` controls how many previous text lines Vinput sends to the LLM as extra context before rewriting. This is useful for translation, continuation, and style-consistent polishing. Set it to `0` to disable extra context.

### CLI

```bash
vinput scene list               # List all scenes
vinput scene add --id <id>      # Add a scene
vinput scene edit <id>          # Edit a scene
vinput scene use-dictation <id> # Set the scene used by dictation (`use` also works)
vinput scene use-command <id>   # Set the scene used by command mode
vinput scene remove <id>        # Remove an inactive scene
```

`scene use-dictation` selects the scene used for ordinary voice input. The former `scene use`
name remains as a compatibility alias. `scene use-command` independently selects the scene used
after pressing the command key, and `scene list` marks both selections.
Switch away from a scene before removing it from either active role.

When adding a scene with LLM, `--provider`, `--model`, and `--prompt` must all be provided.

## LLM providers

### Concept

An LLM provider is an OpenAI-compatible API endpoint. Scenes reference it via `provider_id`.

You can configure multiple providers — different scenes can bind to different providers.

Corresponding config:

```json
{
  "llm": {
    "providers": [
      {
        "id": "groq",
        "base_url": "https://api.groq.com/openai/v1",
        "api_key": "your-api-key"
      }
    ]
  }
}
```

### Setup example

Using local [Ollama](https://ollama.com):

```bash
vinput llm add ollama --base-url http://127.0.0.1:11434/v1
vinput scene add --id polish \
  --label "Polish" \
  --provider ollama \
  --model qwen2.5:7b \
  --context-lines 3 \
  --prompt "Rewrite the recognized text into polished Chinese."
vinput scene use-dictation polish
```

### CLI

```bash
vinput llm list                         # List configured providers
vinput llm add <id> --base-url <url>    # Add
vinput llm edit <id> --base-url <url>   # Edit
vinput llm remove <id>                  # Remove
```

## LLM adapters

### Concept

If your local model does not provide an OpenAI-compatible API directly, install an **LLM adapter**. An adapter is a local process that wraps the model into an OpenAI-compatible API, then you point your LLM provider's `base_url` to it.

Corresponding config:

```json
{
  "llm": {
    "adapters": []
  }
}
```

### GUI

In Vinput GUI, go to **Resources → LLM Adapters** to browse and install.

### CLI

```bash
vinput adapter list             # List installed adapters
vinput adapter list -a          # List available remote adapters
vinput adapter add <id>         # Install
vinput adapter start <id>       # Start
vinput adapter stop <id>        # Stop
```

## Command mode

### Concept

Command mode is a special scene usage: select existing text, then use a voice instruction to have LLM rewrite it.

Flow: select text → hold `Control_R` → speak your instruction → release → done.

By default, command mode uses the built-in `__command__` scene. Select another existing scene without changing the ordinary voice-input scene:

```bash
vinput scene use-command <id>
```

The selected scene supplies command mode's stored prompt, provider, model, candidate count, and
timeout; the audio supplies the spoken instruction after ASR. A prompt can place the selected text
and spoken instruction with `{{selected}}` and `{{asr}}`. The built-in prompt wraps those values in
Vinput-scoped XML tags (`<vinput-selected>` and `<vinput-asr>`); prompts without interpolation
placeholders receive equivalent XML blocks appended at runtime.

**Examples:**
- Select Chinese text → say *"translate to English"* → replaced with translation
- Select code → say *"add comments"* → replaced with commented version

If there is no surrounding-text selection, it falls back to primary-selection clipboard content.

> Command mode requires an LLM-ready active command scene: its `provider_id`, `model`, and
> `prompt` must be configured, and `candidate_count` must be greater than `0`.
