#!/usr/bin/env bash
# Precompress static assets with Brotli (preferred) and gzip (fallback)
# Run after building: .wasm/compress.sh /path/to/build/output
set -euo pipefail
BUILD_DIR="${1:?Usage: compress.sh <build-dir>}"
for f in "$BUILD_DIR"/librecad.wasm "$BUILD_DIR"/librecad.js "$BUILD_DIR"/librecad.data "$BUILD_DIR"/qtloader.js; do
    [ -f "$f" ] || continue
    echo "Compressing $(basename "$f")..."
    brotli -q 11 "$f" -o "$f.br" 2>/dev/null || echo "  brotli skipped (not installed)"
    gzip -kf "$f"
    echo "  $(du -h "$f" | cut -f1) -> $(du -h "$f.br" 2>/dev/null | cut -f1) (br), $(du -h "$f.gz" | cut -f1) (gz)"
done
echo "Done. Serve with nginx configured for Content-Encoding."
