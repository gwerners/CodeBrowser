#!/bin/bash
# CodeBrowser - Build and run script
#
# Usage:
#   ./run.bash                  # build with Lucy full-text search
#   ./run.bash noFullTextIndex  # build without Lucy, use grep/ripgrep

set -euo pipefail
cd "$(dirname "${0}")"
ROOT=$(pwd)

USE_LUCY=ON
for arg in "$@"; do
  case "$arg" in
    noFullTextIndex) USE_LUCY=OFF ;;
  esac
done

function CheckNeeded {
  if ! command -v "${1}" &>/dev/null; then
    echo "${1} is required but not installed!"
    exit 1
  fi
}

function buildLucyLibs {
  if [[ "$USE_LUCY" != "ON" ]]; then
    echo "=== Skipping Lucy (noFullTextIndex) ==="
    return
  fi

  # Lucy sources are fetched by cmake into build/_deps/
  local CLOWNFISH_SRC="$ROOT/build/_deps/lucy_clownfish-src"
  local LUCY_SRC="$ROOT/build/_deps/lucy-src"

  if [[ ! -d "$CLOWNFISH_SRC" ]]; then
    echo "ERROR: Lucy sources not found. Run cmake first."
    return 1
  fi

  mkdir -p "$ROOT/env"

  # Build Clownfish runtime
  if [[ ! -f "$ROOT/env/lib/libclownfish.so.0.6.0" ]]; then
    echo "=== Building Clownfish runtime ==="
    pushd "$CLOWNFISH_SRC/runtime/c" > /dev/null
      ./configure --prefix="$ROOT/env"
      make && make install
    popd > /dev/null
  else
    echo "  Clownfish runtime already built, skipping"
  fi

  # Build Clownfish compiler
  if [[ ! -f "$ROOT/env/bin/cfc" ]]; then
    echo "=== Building Clownfish compiler ==="
    pushd "$CLOWNFISH_SRC/compiler/c" > /dev/null
      ./configure --prefix="$ROOT/env"
      make && make install
    popd > /dev/null
  else
    echo "  Clownfish compiler already built, skipping"
  fi

  # Build Lucy
  if [[ ! -f "$ROOT/env/lib/liblucy.so.0.6.0" ]]; then
    echo "=== Building Lucy ==="
    pushd "$LUCY_SRC/c" > /dev/null
      export PATH="$PATH:$ROOT/env/bin"
      ./configure --clownfish-prefix="$ROOT/env" --prefix="$ROOT/env"
      make && make install
      cp -R autogen/include "$ROOT/env"
    popd > /dev/null
  else
    echo "  Lucy already built, skipping"
  fi
}

function build {
  echo "=== Configuring CodeBrowser (USE_LUCY=$USE_LUCY) ==="
  mkdir -p build
  pushd build > /dev/null
    cmake .. -DUSE_LUCY=$USE_LUCY
  popd > /dev/null

  # Build Lucy libs from fetched sources (if enabled)
  buildLucyLibs

  echo "=== Building CodeBrowser ==="
  pushd build > /dev/null
    make -j$(nproc)
  popd > /dev/null
}

function run {
  if [[ "$USE_LUCY" == "ON" && -d "$ROOT/env/lib" ]]; then
    export LD_LIBRARY_PATH="$ROOT/env/lib:${LD_LIBRARY_PATH:-}"
  fi
  echo "=== Starting CodeBrowser ==="
  ./build/work/codebrowser
}

# --- Main ---
CheckNeeded g++
CheckNeeded cmake
CheckNeeded git

build
run
