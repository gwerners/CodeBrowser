#!/bin/bash
# CodeBrowser - Clean build artifacts

set -euo pipefail
cd "$(dirname "${0}")"

echo "Removing build env and index"
rm -rf build
rm -rf env
rm -rf index


echo "Clean done."
