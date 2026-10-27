#!/usr/bin/env bash
# ==============================================================================
# akernel Test Runner Script
# Compiles and runs unit tests (AASH interpreter test suite).
# ==============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "[INFO] Running akernel test suite..."
"${SCRIPT_DIR}/build.sh" test "$@"
