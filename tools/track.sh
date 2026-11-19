#!/usr/bin/env bash
set -euo pipefail

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
KEY_FILE="${1:-/tmp/mykeys/private.key}"

if [ ! -f "$KEY_FILE" ]; then
    echo "Error: Private key file '$KEY_FILE' not found!"
    echo "Usage: ./tools/track.sh [path_to_private.key]"
    exit 1
fi

export PATH="/tmp/findmy_venv/bin:$PATH"

if [ ! -f "/tmp/findmy_venv/bin/python3" ]; then
    echo "[*] Setting up one-time isolated FindMy environment..."
    nix-shell -p python3Packages.pip python3Packages.virtualenv --run "
        python3 -m venv /tmp/findmy_venv
        /tmp/findmy_venv/bin/pip install --quiet findmy
    "
fi

exec /tmp/findmy_venv/bin/python3 "$DIR/tools/fetch.py" "$KEY_FILE"
