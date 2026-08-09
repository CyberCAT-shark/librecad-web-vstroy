#!/usr/bin/env bash
set -euo pipefail
BUILD_DIR="${1:?Usage: deploy.sh <build-dir>}"
DEPLOY_DIR="${2:-/var/www/librecad}"
echo "Deploying LibreCAD WASM to $DEPLOY_DIR"
mkdir -p "$DEPLOY_DIR"
cp "$BUILD_DIR"/librecad.{wasm,js,html,data} "$BUILD_DIR"/qtloader.js "$DEPLOY_DIR"/
# Precompress
"$(dirname "$0")/compress.sh" "$DEPLOY_DIR"
echo "Done. Configure nginx with .wasm/nginx.conf."
