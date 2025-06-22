#!/bin/bash -x

argc=$#

#https://vaneyckt.io/posts/safer_bash_scripts_with_set_euxo_pipefail/
set -euo pipefail

#show commands (for debuging)
#set -x

#move to script dir 
cd "$(dirname "${0}")"

#save current dir as base dir
ROOT=$(pwd)

function safeRM {
  for file in ${@} ; do
    if [[ -f "${file}" ]]; then
      rm "${file}"
    fi
  done
}
function safeRMDIR {
  for dir in ${@} ; do
    if [[ -d "${dir}" ]]; then
      rm -rf "${dir}"
    fi
  done
}

function clear {
  safeRMDIR build
  safeRMDIR env
  safeRMDIR lucy lucy-clownfish
  safeRMDIR .cache
  safeRM *.user
}
###############################################################################
#script main:
clear

