#!/usr/bin/env python3
"""Post-process the Emscripten-generated librecad.js for the JSPI build.

Qt for WebAssembly suspends the calling wasm stack (for QDialog::exec(),
combobox popups, nested event loops, blocking file/settings I/O, ...) via a
WebAssembly.Suspending import. Under JSPI a suspend only works if the current
call stack was entered through a WebAssembly.promising frame.

Emscripten marks only `main` as promising. DOM events are made promising by a
Qt source patch (EventListener::handleEvent registered with emscripten::async()).
The remaining JS->wasm entry points that can reach a Qt suspend are the timer
and posted-event callbacks scheduled through emscripten_set_timeout() and
emscripten_async_call() (e.g. QEventDispatcherWasm::callProcessTimers, which
runs Qt timer slots directly in the timeout callback). Those callbacks are
plain wasm function pointers invoked via getWasmTableEntry(); their return
value is ignored, so wrapping them in WebAssembly.promising() is safe and makes
them promising frames too.

This script rewrites those two glue functions in place. It is idempotent and
fails loudly if the expected Emscripten code shape is not found (e.g. after an
Emscripten upgrade), so the build breaks instead of silently shipping a wasm
that deadlocks/aborts on the first nested suspend.
"""
import sys

REPLACEMENTS = [
    # emscripten_set_timeout: wrap the callback in WebAssembly.promising
    (
        "_emscripten_set_timeout=function(cb,msecs,userData){cb>>>=0;userData>>>=0;"
        "return safeSetTimeout(()=>getWasmTableEntry(cb)(userData),msecs)}",
        "_emscripten_set_timeout=function(cb,msecs,userData){cb>>>=0;userData>>>=0;"
        "return safeSetTimeout(()=>WebAssembly.promising(getWasmTableEntry(cb))(userData),msecs)}",
    ),
    # emscripten_async_call: wrap the callback in WebAssembly.promising
    (
        "_emscripten_async_call=function(func,arg,millis){func>>>=0;arg>>>=0;"
        "var wrapper=()=>getWasmTableEntry(func)(arg);",
        "_emscripten_async_call=function(func,arg,millis){func>>>=0;arg>>>=0;"
        "var wrapper=()=>WebAssembly.promising(getWasmTableEntry(func))(arg);",
    ),
]


def main(path):
    with open(path, "r") as f:
        src = f.read()

    changed = 0
    for old, new in REPLACEMENTS:
        if new in src:
            # already patched (idempotent)
            changed += 1
            continue
        n = src.count(old)
        if n != 1:
            sys.stderr.write(
                f"jspi_postprocess: expected exactly 1 match for a pattern, found {n}.\n"
                f"  Emscripten output shape changed; update .wasm/jspi_postprocess.py.\n"
                f"  Pattern: {old[:80]}...\n"
            )
            return 2
        src = src.replace(old, new)
        changed += 1

    if changed != len(REPLACEMENTS):
        sys.stderr.write("jspi_postprocess: not all patterns applied\n")
        return 2

    with open(path, "w") as f:
        f.write(src)
    print(f"jspi_postprocess: patched {path} (timer/async_call -> WebAssembly.promising)")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.stderr.write("usage: jspi_postprocess.py <path-to-librecad.js>\n")
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
