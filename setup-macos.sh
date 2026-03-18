#!/usr/bin/env bash
set -euo pipefail

# ─────────────────────────────────────────────────────────────────────────────
#  dune3d macOS dependency installer
# ─────────────────────────────────────────────────────────────────────────────

# ANSI colors & styles
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

BLACK='\033[0;30m'
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[0;37m'

BRED='\033[1;31m'
BGREEN='\033[1;32m'
BYELLOW='\033[1;33m'
BBLUE='\033[1;34m'
BMAGENTA='\033[1;35m'
BCYAN='\033[1;36m'
BWHITE='\033[1;37m'

BG_BLUE='\033[44m'
BG_MAGENTA='\033[45m'

# ─── Helpers ─────────────────────────────────────────────────────────────────

print_banner() {
  echo
  echo -e "${BG_BLUE}${BWHITE}                                                       ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ██████╗ ██╗   ██╗███╗   ██╗███████╗██████╗ ██████╗  ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ██╔══██╗██║   ██║████╗  ██║██╔════╝╚════██╗██╔══██╗ ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ██║  ██║██║   ██║██╔██╗ ██║█████╗   █████╔╝██║  ██║ ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ██║  ██║██║   ██║██║╚██╗██║██╔══╝   ╚═══██╗██║  ██║ ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ██████╔╝╚██████╔╝██║ ╚████║███████╗██████╔╝██████╔╝ ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}  ╚═════╝  ╚═════╝ ╚═╝  ╚═══╝╚══════╝╚═════╝ ╚═════╝ ${RESET}"
  echo -e "${BG_BLUE}${BWHITE}                                                       ${RESET}"
  echo -e "${DIM}${CYAN}               macOS dependency installer               ${RESET}"
  echo
}

step() {
  echo -e "\n${BOLD}${BBLUE}┌─${RESET}${BOLD} $1${RESET}"
}

info() {
  echo -e "${CYAN}│  ${WHITE}$1${RESET}"
}

ok() {
  echo -e "${BGREEN}│  ✓ ${GREEN}$1${RESET}"
}

warn() {
  echo -e "${BYELLOW}│  ⚠ ${YELLOW}$1${RESET}"
}

fail() {
  echo -e "${BRED}│  ✗ ${RED}$1${RESET}"
  exit 1
}

divider() {
  echo -e "${DIM}${BLUE}└────────────────────────────────────────────────────${RESET}"
}

pkg_status() {
  local pkg="$1"
  if brew list "$pkg" &>/dev/null; then
    echo -e "${GREEN}  ✓${RESET} ${WHITE}${pkg}${RESET} ${DIM}(already installed)${RESET}"
  else
    echo -e "${YELLOW}  ○${RESET} ${WHITE}${pkg}${RESET} ${DIM}(will install)${RESET}"
  fi
}

install_pkg() {
  local pkg="$1"
  if brew list "$pkg" &>/dev/null; then
    echo -e "${GREEN}  ✓ ${pkg}${RESET} ${DIM}skipped${RESET}"
  else
    echo -e "${CYAN}  → installing ${BOLD}${pkg}${RESET}${CYAN}...${RESET}"
    if brew install "$pkg" &>/dev/null; then
      echo -e "${BGREEN}  ✓ ${pkg}${RESET}"
    else
      echo -e "${BRED}  ✗ ${pkg} failed — check brew output above${RESET}"
      exit 1
    fi
  fi
}

# ─── Start ───────────────────────────────────────────────────────────────────

print_banner

# ─── Check: macOS ────────────────────────────────────────────────────────────
step "Checking platform"
if [[ "$(uname)" != "Darwin" ]]; then
  fail "This script is for macOS only."
fi
ok "macOS $(sw_vers -productVersion)"
divider

# ─── Check: Homebrew ─────────────────────────────────────────────────────────
step "Checking Homebrew"
if ! command -v brew &>/dev/null; then
  warn "Homebrew not found — installing..."
  /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  # Add brew to PATH for Apple Silicon
  if [[ -f /opt/homebrew/bin/brew ]]; then
    eval "$(/opt/homebrew/bin/brew shellenv)"
  fi
fi
BREW_PREFIX="$(brew --prefix)"
ok "Homebrew at ${BREW_PREFIX}"
divider

# ─── Preview ─────────────────────────────────────────────────────────────────
step "Packages to install"

echo -e "\n  ${BMAGENTA}Build tools${RESET}"
for p in meson ninja cmake pkg-config; do pkg_status "$p"; done

echo -e "\n  ${BMAGENTA}Python + GObject bindings (for icon atlas generator)${RESET}"
for p in python@3.13 gobject-introspection pygobject3; do pkg_status "$p"; done

echo -e "\n  ${BMAGENTA}GTK / UI${RESET}"
for p in gtk4 gtkmm4 libepoxy librsvg cairo; do pkg_status "$p"; done

echo -e "\n  ${BMAGENTA}Math / geometry${RESET}"
for p in eigen glm opencascade; do pkg_status "$p"; done

echo -e "\n  ${BMAGENTA}Fonts / graphics${RESET}"
for p in harfbuzz freetype libpng; do pkg_status "$p"; done

echo -e "\n  ${BMAGENTA}Misc${RESET}"
for p in ossp-uuid; do pkg_status "$p"; done

echo
divider

# ─── Confirm ─────────────────────────────────────────────────────────────────
step "Ready to install"
info "This may take a few minutes on a fresh machine."
echo
echo -ne "  ${BYELLOW}Proceed? [y/N] ${RESET}"
read -r confirm
if [[ ! "$confirm" =~ ^[Yy]$ ]]; then
  echo -e "\n  ${DIM}Aborted.${RESET}\n"
  exit 0
fi
divider

# ─── Install ─────────────────────────────────────────────────────────────────
step "Installing build tools"
install_pkg meson
install_pkg ninja
install_pkg cmake
install_pkg pkg-config
divider

step "Installing Python + GObject bindings"
install_pkg python@3.13
install_pkg gobject-introspection
install_pkg pygobject3
divider

step "Installing GTK / UI libraries"
install_pkg gtk4
install_pkg gtkmm4
install_pkg libepoxy
install_pkg librsvg
install_pkg cairo
divider

step "Installing math / geometry libraries"
install_pkg eigen
install_pkg glm
install_pkg opencascade
divider

step "Installing font / graphics libraries"
install_pkg harfbuzz
install_pkg freetype
install_pkg libpng
divider

step "Installing misc libraries"
install_pkg ossp-uuid
divider

# ─── Verify typelib ──────────────────────────────────────────────────────────
step "Verifying GObject typelibs"
TYPELIB_DIR="${BREW_PREFIX}/lib/girepository-1.0"
if [[ -f "${TYPELIB_DIR}/cairo-1.0.typelib" ]]; then
  ok "cairo-1.0.typelib found"
else
  warn "cairo typelib not found — trying to reinstall gobject-introspection"
  brew reinstall gobject-introspection &>/dev/null
  if [[ -f "${TYPELIB_DIR}/cairo-1.0.typelib" ]]; then
    ok "cairo-1.0.typelib found after reinstall"
  else
    warn "cairo typelib still missing — build may fail (run: brew reinstall cairo gobject-introspection)"
  fi
fi
if [[ -f "${TYPELIB_DIR}/Rsvg-2.0.typelib" ]]; then
  ok "Rsvg-2.0.typelib found"
else
  warn "Rsvg typelib not found — run: brew reinstall librsvg"
fi
divider

# ─── Build instructions ──────────────────────────────────────────────────────
PYTHON3="${BREW_PREFIX}/bin/python3"

step "All done! Build instructions"
echo
echo -e "  ${BYELLOW}Configure:${RESET}"
echo -e "  ${DIM}${WHITE}PATH=${BREW_PREFIX}/bin:\$PATH meson setup build${RESET}"
echo
echo -e "  ${BYELLOW}Compile:${RESET}"
echo -e "  ${DIM}${WHITE}GI_TYPELIB_PATH=${TYPELIB_DIR} meson compile -C build${RESET}"
echo
echo -e "  ${BYELLOW}Or as a one-liner:${RESET}"
echo -e "  ${DIM}${WHITE}PATH=${BREW_PREFIX}/bin:\$PATH meson setup build && GI_TYPELIB_PATH=${TYPELIB_DIR} meson compile -C build${RESET}"
echo
echo -e "  ${DIM}Tip: add${RESET} ${CYAN}export GI_TYPELIB_PATH=${TYPELIB_DIR}${RESET}"
echo -e "  ${DIM}to your${RESET} ${WHITE}~/.zshrc${RESET} ${DIM}to avoid typing it each time.${RESET}"
echo
echo -e "${BGREEN}  ★ Happy building! ★${RESET}"
echo
divider
echo
