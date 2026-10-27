#!/usr/bin/env bash
# Forward to scripts/qemu-run.sh
exec "$(dirname "$0")/qemu-run.sh" "$@"
