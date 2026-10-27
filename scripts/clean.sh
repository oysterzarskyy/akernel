#!/usr/bin/env bash
# ==============================================================================
# akernel Clean Script
# Cleans build artifacts and temporary files.
# ==============================================================================

set -eo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

echo "[INFO] Cleaning build directory..."
"${SCRIPT_DIR}/build.sh" clean "$@"
