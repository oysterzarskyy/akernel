#!/usr/bin/env bash
# Forward to scripts/build.sh
exec "$(dirname "$0")/scripts/build.sh" "$@"
