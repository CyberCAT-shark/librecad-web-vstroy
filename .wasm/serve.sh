#!/usr/bin/env bash
set -euo pipefail
SERVE_DIR="${1:-/build}"
PORT="${2:-8000}"
python3 - "$SERVE_DIR" "$PORT" << 'PYEOF'
import http.server, socketserver, os, sys
serve_dir, port = sys.argv[1], int(sys.argv[2])
class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        super().end_headers()
    def guess_type(self, path):
        if path.endswith(".wasm"):
            return "application/wasm"
        return super().guess_type(path)
os.chdir(serve_dir)
with socketserver.TCPServer(("", port), Handler) as s:
    print(f"Serving {serve_dir} on http://0.0.0.0:{port}")
    s.serve_forever()
PYEOF
