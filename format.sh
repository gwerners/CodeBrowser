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

update_code() {
    # Set default style if no parameter is provided
    if [ $# -eq 0 ]; then
        style="Chromium"  # Default style
    else
        style="$1"
    fi

    # Print available styles
    echo "Available styles: LLVM, GNU, Google, Chromium, Microsoft, Mozilla, WebKit"

    # Get the modification time of the current script
    script_mtime=$(stat -c %Y "$0")

    # Define directories to be ignored
    ignore_dirs=(asio sol fmt fmt-src crow nlohmann ccls build)

    # Construct the find command to exclude specified directories
    find_command="find . \( -name '*.cxx' -o -name '*.cpp' -o -name '*.h' -o -name '*.hpp' \)"
    for dir in "${ignore_dirs[@]}"; do
        find_command+=" -not -path \"*/$dir/*\""
    done

    # Find all C/C++ header and source files, excluding specified directories
    files=$(eval "$find_command" -newermt @$script_mtime)
    #files=$(eval "$find_command") #all!

    # Run clang-format on each file
    for file in $files; do
        echo "Formatting: $file"
        clang-format -style="$style" -i "$file"
    done

    echo "Finished."
    # Update modification time of the current script
    touch "$0"
}





###############################################################################
#script main:
update_code "$@"

