#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

if ! command -v pandoc >/dev/null 2>&1; then
    echo "Error: pandoc is required to generate man pages." >&2
    exit 1
fi

echo "Generating man pages with pandoc..."

for lang in en zh_CN; do
    echo "  Processing [${lang}]..."
    pandoc --from=markdown-smart -s -t man \
        "${ROOT_DIR}/data/man/${lang}/vinput.1.md" \
        -o "${ROOT_DIR}/data/man/${lang}/vinput.1"
    
    pandoc --from=markdown-smart -s -t man \
        "${ROOT_DIR}/data/man/${lang}/vinput-daemon.1.md" \
        -o "${ROOT_DIR}/data/man/${lang}/vinput-daemon.1"
    
    pandoc --from=markdown-smart -s -t man \
        "${ROOT_DIR}/data/man/${lang}/vinput-gui.1.md" \
        -o "${ROOT_DIR}/data/man/${lang}/vinput-gui.1"
    
    pandoc --from=markdown-smart -s -t man \
        "${ROOT_DIR}/data/man/${lang}/vinput-config.5.md" \
        -o "${ROOT_DIR}/data/man/${lang}/vinput-config.5"

    pandoc --from=markdown-smart -s -t man \
        "${ROOT_DIR}/data/man/${lang}/fcitx5-vinput.7.md" \
        -o "${ROOT_DIR}/data/man/${lang}/fcitx5-vinput.7"
done

echo "Man pages generated successfully."
