#!/usr/bin/env bash
set -euo pipefail
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
docker run -it --rm \
    -v "$REPO_ROOT:/work" \
    -v lcad-wasm-build:/build \
    -p 8000:8000 \
    lcad-wasm:6.8 bash
