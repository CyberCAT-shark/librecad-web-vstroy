FROM ubuntu:24.04

ARG QT_VERSION=6.8.3
ARG EMSDK_VERSION=3.1.74
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build python3 python3-pip \
        git curl ca-certificates libxkbcommon0 zlib1g-dev \
        libgl1-mesa-dev libfontconfig1 libdbus-1-3 libfreetype6-dev \
        libglib2.0-0 pkg-config xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --break-system-packages aqtinstall

# Host Qt (Linux gcc_64) — needed as QT_HOST_PATH for cross-compiling
RUN aqt install-qt -O /opt/Qt linux desktop ${QT_VERSION} linux_gcc_64

# Qt source (qtbase + qtsvg only, to minimize build time)
RUN aqt install-src -O /opt/Qt linux ${QT_VERSION} --archives qtbase qtsvg

# Emscripten SDK — 3.1.74 (Qt 6.8 requires >=3.1.56; using newer for
# matching ABI with from-source Qt build)
RUN git clone https://github.com/emscripten-core/emsdk.git /opt/emsdk \
    && /opt/emsdk/emsdk install ${EMSDK_VERSION} \
    && /opt/emsdk/emsdk activate ${EMSDK_VERSION} \
    && rm -rf /opt/emsdk/.git

ENV EMSDK=/opt/emsdk
ENV QT_HOST_PATH=/opt/Qt/${QT_VERSION}/gcc_64
ENV PATH=/opt/emsdk:/opt/emsdk/upstream/emscripten:${PATH}

# Boost headers (isolated symlink to avoid sysroot conflicts)
RUN apt-get update && apt-get install -y --no-install-recommends libboost-dev \
    && mkdir -p /opt/boost/include && ln -s /usr/include/boost /opt/boost/include/boost \
    && rm -rf /var/lib/apt/lists/*

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh

SHELL ["/bin/bash", "-c"]

# Build Qt from source with classic Asyncify (QT_EMSCRIPTEN_ASYNCIFY=1).
# This sets QT_HAVE_EMSCRIPTEN_ASYNCIFY compile definition in Qt's source,
# making Qt's event loop (QEventLoop::exec, QDialog::exec, QMenu::exec) use
# the asyncify-aware implementation that properly cooperates with Emscripten's
# suspend/resume mechanism. Without this, nested event loops (second-level
# context menus, dialogs from dialogs) fail.
#
# Qt's toolchain sets -sASYNCIFY -Os via INTERFACE_LINK_OPTIONS automatically.
#
# NOTE: No Qt source patches. Investigation (see git history) showed that
# nested/second-level popup menus work correctly in stock Qt 6.8.3 built with
# QT_EMSCRIPTEN_ASYNCIFY=1. Earlier "fix" patches to the wasm event routing
# were based on a misdiagnosis (testing a submenu whose items were legitimately
# disabled) and actually broke nested menu hover; they have been removed.

RUN source /opt/emsdk/emsdk_env.sh \
    && cd /opt/Qt/${QT_VERSION}/Src/qtbase \
    && ./configure \
        -qt-host-path /opt/Qt/${QT_VERSION}/gcc_64 \
        -platform wasm-emscripten \
        -prefix /opt/Qt/${QT_VERSION}/wasm_asyncify \
        -device-option QT_EMSCRIPTEN_ASYNCIFY=1 \
        -nomake examples \
        -nomake tests \
        -release \
    && cmake --build . --parallel $(nproc) \
    && cmake --install . \
    && rm -rf /opt/Qt/${QT_VERSION}/Src/qtbase/CMakeFiles \
             /opt/Qt/${QT_VERSION}/Src/qtbase/src

# Build qtsvg module against the asyncify-enabled Qt
RUN source /opt/emsdk/emsdk_env.sh \
    && cd /opt/Qt/${QT_VERSION}/Src/qtsvg \
    && /opt/Qt/${QT_VERSION}/wasm_asyncify/bin/qt-cmake -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
    && cmake --build . --parallel $(nproc) \
    && cmake --install . \
    && rm -rf /opt/Qt/${QT_VERSION}/Src/qtsvg

ENV QT_WASM_PATH=/opt/Qt/${QT_VERSION}/wasm_asyncify

WORKDIR /work
ENTRYPOINT ["/entrypoint.sh"]
