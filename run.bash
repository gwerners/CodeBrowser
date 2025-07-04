#!/bin/bash

argc=$#

#sudo apt install clang libclang-dev llvm llvm-dev clangd

#https://vaneyckt.io/posts/safer_bash_scripts_with_set_euxo_pipefail/
set -euo pipefail

#show commands (for debuging)
#set -x

#move to script dir 
cd "$(dirname "${0}")"

#save current dir as base dir
ROOT=$(pwd)

#where to store/get tar.gz / tar.bz2
DEPENDENCIES=../dependencies

#avoid rm error if file doesn't exists
function safeRM {
  for file in ${@} ; do
    if [[ -f "${file}" ]]; then
      rm "${file}"
    fi
  done
}
#avoid rm error if directory doesn't exists
function safeRMDIR {
  for dir in ${@} ; do
    if [[ -d "${dir}" ]]; then
      rm -rf "${dir}"
    fi
  done
}
function safeMKDIR {
  if [[ ! -d "$1" ]]; then
    mkdir "$1"
  fi
}
function TAR {
  safeMKDIR "${DEPENDENCIES}"
  if [[ -f "${DEPENDENCIES}/$1.tar.gz" ]]; then
    echo "skipped"
    return
  else
    tar -cjf $1.tar.bz2 $1
    mv $1.tar.bz2 ${DEPENDENCIES}/
    echo "done"
    return
  fi
}
function UNTAR {
  if [[ ! -d "$1" ]]; then
    if [[ ! -f "${DEPENDENCIES}/$1.tar.gz" ]] && [[ ! -f "${DEPENDENCIES}/$1.tar.bz2" ]]; then
      echo "skipped"
      return
    else
      tar -xf ${DEPENDENCIES}/$1.tar.*
      echo "done"
      return
    fi
  fi
}
function CheckNeeded {
  NEEDED=$(which ${1})
  if [ ! -f "${NEEDED}" ]; then
    echo "${1} is a dependency, you must install it first!"
    exit 0
  fi
}
function cleanOldBuild {
  ./clear.sh
}
function get {
  untarResult=$(UNTAR "$1")
  if [[ "${untarResult}"  == "skipped" ]]; then 
    if [[ ! -d "$1" ]]; then
      git clone "$2"
      TAR "$1"
      cd ${ROOT}
    fi
  fi
}
function getAll {
  echo "get dependencies"
  get fmt https://github.com/fmtlib/fmt.git
  #get lsp-cpp https://github.com/alextsao1999/lsp-cpp.git
  #get Sockets https://github.com/GavinDistaso/Sockets.git
  #get monaco-editor https://github.com/microsoft/monaco-editor.git
  #get dev https://github.com/codemirror/dev.git
  #get clangd-in-browser https://github.com/Guyutongxue/clangd-in-browser.git
  #get monaco-editor-cpp https://github.com/wxydev1/monaco-editor-cpp.git
  #get wsServer https://github.com/Theldus/wsServer.git
  #get clangd https://github.com/clangd/clangd
  #get llvm-project https://github.com/llvm/llvm-project.git
  #get Crow https://github.com/CrowCpp/Crow.git
  #get asio https://github.com/chriskohlhoff/asio.git
  #get compiledb https://github.com/nickdiego/compiledb.git
  #get sol2 https://github.com/ThePhD/sol2.git
  #get tiny-regex-c https://github.com/kokke/tiny-regex-c.git
  get lucy-clownfish https://github.com/apache/lucy-clownfish.git
  get lucy https://github.com/apache/lucy.git
 
}
function buildClownfish {
  safeMKDIR "$ROOT/env"
  pushd lucy-clownfish
    if [[ ! -f "$ROOT/env/lib/libclownfish.so.0.6.0" ]]; then
      pushd runtime/c
        ./configure --prefix="$ROOT/env"
        make
        make install
      popd
    fi
    if [[ ! -f "$ROOT/env/bin/cfc" ]]; then
      pushd compiler/c
        ./configure --prefix="$ROOT/env"
        make
        make install
        #safeMKDIR "$ROOT/env/include"
        #$ROOT/env/bin/cfc --include=$ROOT/env/share/clownfish/include --parcel=Lucy --dest="$ROOT/env"
      popd
    fi
  popd
}
function buildLucy {
  if [[ ! -f "$ROOT/env/lib/liblucy.so.0.6.0" ]]; then
    safeMKDIR "$ROOT/env"
    pushd lucy
      pushd c
        export PATH="$PATH:$ROOT/env/bin"
        ./configure --clownfish-prefix="$ROOT/env"  --prefix="$ROOT/env"
        make
        make install
        cp -R autogen/include "$ROOT/env"
      popd
    popd
  fi
}

function buildMonaco {
  #you don't need to build monaco since min is already installed in root directory...
  if [ -f "root/monaco/min/vs/editor/editor.main.js" ]; then
    return
  fi
  get monaco-editor git@github.com:gwerners/monaco-editor.git
  if [ -d "monaco-editor" ]; then
    pushd monaco-editor
      npm install monaco-editor
      if [ -d "root/monaco/min" ]; then
        rm -rf "root/monaco/min"
        cp -r "monaco-editor/node_modules/monaco-editor/min" "root/monaco/min"
      fi
    popd
  fi
  rm -rf monaco-editor
}

function build {
  buildClownfish
  buildLucy
  #buildMonaco
  
  safeMKDIR build
  pushd build
    cmake ..
    make all
  popd
<<'#COMMENT_BLOCK'
bla

cat <<EOF > .clangd
CompileFlags:
  # Treat code as C++, use C++17 standard, enable more warnings.
  Add: [-xc++, -std=c++17, -Wall, -Wno-missing-prototypes]
  # Remove extra warnings specified in compile commands.
  # Single value is also acceptable, same as "Remove: [-mabi]"
  Remove: -mabi
Diagnostics:
  # Tweak Clang-Tidy checks.
  ClangTidy:
    Add: [performance*, modernize*, readability*]
    Remove: [modernize-use-trailing-return-type]
    CheckOptions:
      readability-identifier-naming.VariableCase: CamelCase
---
# Use Remote Index Service for LLVM.
If:
  # Note: This is a regexp, notice '.*' at the end of PathMatch string.
  PathMatch: /path/to/llvm/.*
Index:
  External:
    Server: clangd-index.llvm.org:5900
    MountPoint: /path/to/llvm/
EOF
#COMMENT_BLOCK

}
function run {
  #need llvm path for llvm-symbolizer
  export PATH="$PATH:/usr/lib/llvm-18/bin"
  ./build/work/lsp-client
}
###############################################################################
#script main:
CheckNeeded g++ || exit 1
CheckNeeded cmake || exit 1
CheckNeeded git  || exit 1
#cleanOldBuild
getAll
build
run

