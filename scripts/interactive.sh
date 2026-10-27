#!/usr/bin/env bash
# ------------------------------------------------------------
# interactive.sh – Minimal interactive terminal wrapper for akernel
# ------------------------------------------------------------
# Provides a Linux‑like prompt with colors, command parsing, and
# simple alerts.  Commands are executed synchronously but the user
# sees progress messages rather than an instant jump.
# ------------------------------------------------------------

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "${REPO_ROOT}"

# ANSI colour codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
RESET='\033[0m'

# Prompt – shows "@user/main $" with colours
USER_NAME='${USER:-User}'
HOST_NAME='${HOSTNAME:-local}'
PWD_SHORT=$(basename "$PWD")
PS1="${GREEN}@${USER_NAME}${RESET}/${MAGENTA}${PWD_SHORT}${RESET} \$ "

# Helper to print an alert banner
alert() {
  local level=$1
  shift
  local msg="$*"
  case "$level" in
    info)   colour="$CYAN";;
    warn)   colour="$YELLOW";;
    error)  colour="$RED";;
    *)      colour="$RESET";;
  esac
  echo -e "${BOLD}${colour}[${level^^}]${RESET} ${msg}"
}

# Command dispatcher
run_command() {
  case "$1" in
    build|b)
      alert info "Starting kernel build…"
      ./scripts/build.sh all || { alert error "Build failed"; return; }
      alert info "Build finished"
      ;;
    iso|i)
      alert info "Building ISO…"
      ./scripts/build.sh iso || { alert error "ISO build failed"; return; }
      alert info "ISO created"
      ;;
    run|r)
      alert info "Launching QEMU…"
      ./scripts/qemu-run.sh || { alert error "QEMU failed"; return; }
      alert info "QEMU terminated"
      ;;
    clean|c)
      alert warn "Cleaning build artefacts…"
      ./scripts/build.sh clean
      alert info "Clean complete"
      ;;
    version|v)
      if [[ -f build/VERSION ]]; then
        echo "Version: $(cat build/VERSION)"
      else
        echo "Version file not found"
      fi
      ;;
    exit|quit|q)
      alert info "Good‑bye!"
      exit 0
      ;;
    help|h|?)
      echo -e "${BOLD}Available commands:${RESET}"
      echo "  ${GREEN}build${RESET} (b)    – compile the kernel"
      echo "  ${GREEN}iso${RESET} (i)      – build the ISO image"
      echo "  ${GREEN}run${RESET} (r)      – launch QEMU with the ISO"
      echo "  ${GREEN}clean${RESET} (c)    – clean build artefacts"
      echo "  ${GREEN}version${RESET} (v) – show built version"
      echo "  ${GREEN}exit${RESET} (q)    – leave the terminal"
      ;;
    *)
      alert warn "Unknown command: $1 (type 'help' for list)"
      ;;
  esac
}

# Main REPL loop
while true; do
  # Refresh prompt in case directory changed
  PWD_SHORT=$(basename "$PWD")
  PS1="${GREEN}@${USER_NAME}${RESET}/${MAGENTA}${PWD_SHORT}${RESET} \$ "
  read -rp "$PS1" cmd args
  # Split the line into an array to support arguments in the future
  IFS=' ' read -ra parts <<< "$cmd $args"
  run_command "${parts[0]}" "${parts[@]:1}"
done
