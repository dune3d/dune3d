#!/usr/bin/env bash
set -euo pipefail

# ─────────────────────────────────────────────────────────────────────────────
#  dune3d — macOS App Bundle + DMG Creator
#  Supports: arm64 | x86_64 | universal (fat binary, single-machine build)
# ─────────────────────────────────────────────────────────────────────────────

BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
WHITE='\033[0;37m'

BRED='\033[1;31m'
BGREEN='\033[1;32m'
BYELLOW='\033[1;33m'
BBLUE='\033[1;34m'
BWHITE='\033[1;37m'

BG_BLUE='\033[44m'

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
  echo -e "${DIM}${CYAN}               macOS App Bundle Creator               ${RESET}"
  echo
}

step()     { echo -e "\n${BOLD}${BBLUE}┌─${RESET}${BOLD} $*${RESET}"; }
info()     { echo -e "${CYAN}│  ${WHITE}$*${RESET}"; }
ok()       { echo -e "${BGREEN}│  ✓ ${GREEN}$*${RESET}"; }
warn()     { echo -e "${BYELLOW}│  ⚠ ${YELLOW}$*${RESET}"; }
fail()     { echo -e "${BRED}│  ✗ ${RED}$*${RESET}"; exit 1; }
div()      { echo -e "${DIM}${BBLUE}└────────────────────────────────────────────────────${RESET}"; }
kv()       { printf "${CYAN}│  ${DIM}%-20s${RESET}  ${BWHITE}%s${RESET}\n" "$1" "$2"; }
progress() { echo -ne "${CYAN}│  ${DIM}$* ...${RESET}"; }
done_()    { echo -e " ${BGREEN}done${RESET}"; }

# ─── Path fixing ─────────────────────────────────────────────────────────────

# Fix all Homebrew deps (both arm64 /opt/homebrew and x86 /usr/local)
fix_homebrew_deps() {
  local file="$1"
  local frameworks_path="$2"

  # Fix own install name if it points into a Homebrew prefix
  local own_id
  own_id="$(otool -D "$file" 2>/dev/null | tail -1 || true)"
  if [[ "$own_id" == /opt/homebrew/* ]] || [[ "$own_id" == /usr/local/* ]]; then
    install_name_tool -id "${frameworks_path}/$(basename "$file")" "$file" 2>/dev/null || true
  fi

  # Fix all /opt/homebrew and /usr/local dep references
  local deps
  deps="$(otool -L "$file" 2>/dev/null \
    | awk '{print $1}' \
    | grep -E '^(/opt/homebrew|/usr/local)' || true)"
  [[ -z "$deps" ]] && return 0
  while IFS= read -r dep; do
    local lib_name
    lib_name="$(basename "$dep")"
    install_name_tool -change "$dep" "${frameworks_path}/${lib_name}" "$file" 2>/dev/null || true
  done <<< "$deps"
}

# Remove duplicate LC_RPATH entries, re-sign with ad-hoc
dedup_rpaths_and_sign() {
  local file="$1"
  local all_rpaths
  all_rpaths="$(otool -l "$file" \
    | awk '/LC_RPATH/{found=1} found && /path /{print $2; found=0}' || true)"
  local seen=()
  while IFS= read -r rpath; do
    [[ -z "$rpath" ]] && continue
    local already=false
    for s in "${seen[@]:-}"; do [[ "$s" == "$rpath" ]] && { already=true; break; }; done
    if $already; then
      install_name_tool -delete_rpath "$rpath" "$file" 2>/dev/null || true
    else
      seen+=("$rpath")
    fi
  done <<< "$all_rpaths"
  codesign --force --sign - "$file" 2>/dev/null || true
}

# ─── Args ────────────────────────────────────────────────────────────────────

BUILD=false
ARCH="arm64"        # arm64 | x86_64 | universal
SIGN_ID=""
OUTPUT_DIR="$(pwd)/dist"
SETUP_X86=false

for arg in "$@"; do
  case "$arg" in
    --build)         BUILD=true ;;
    --universal)     ARCH="universal" ;;
    --arch=*)        ARCH="${arg#--arch=}" ;;
    --sign=*)        SIGN_ID="${arg#--sign=}" ;;
    --output=*)      OUTPUT_DIR="${arg#--output=}" ;;
    --setup-x86)     SETUP_X86=true ;;
    --help|-h)
      echo -e "\n${BOLD}Usage:${RESET}  $0 [options]"
      echo
      echo -e "  ${CYAN}--build${RESET}           Rebuild from source before bundling"
      echo -e "  ${CYAN}--universal${RESET}       Build fat binary (arm64 + x86_64)"
      echo -e "  ${CYAN}--arch=ARCH${RESET}       Target arch: arm64 (default), x86_64, universal"
      echo -e "  ${CYAN}--setup-x86${RESET}       Install x86_64 Homebrew + deps (one-time setup)"
      echo -e "  ${CYAN}--sign=IDENTITY${RESET}   Code-sign with given identity"
      echo -e "  ${CYAN}--output=DIR${RESET}      Output directory (default: ./dist)"
      echo; exit 0 ;;
    *) echo -e "${BRED}Unknown argument: $arg${RESET}"; exit 1 ;;
  esac
done

[[ "$ARCH" =~ ^(arm64|x86_64|universal)$ ]] || fail "Invalid --arch: ${ARCH}"

print_banner

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARM_BUILD_DIR="${REPO_DIR}/build"
X86_BUILD_DIR="${REPO_DIR}/build-x86_64"

BREW_ARM="/opt/homebrew"          # Apple Silicon Homebrew
BREW_X86="/usr/local"             # x86_64 Homebrew (Rosetta)
BREW_PREFIX="${BREW_ARM}"         # default; overridden for x86_64-only mode

if [[ "$ARCH" == "x86_64" ]]; then
  BREW_PREFIX="${BREW_X86}"
fi

VERSION_PY="${REPO_DIR}/version.py"
APP_VERSION="$(python3 -c "exec(open('${VERSION_PY}').read()); print(f'{major}.{minor}.{micro}')")"
APP_NAME_QUOTED="$(python3 -c "exec(open('${VERSION_PY}').read()); print(name)")"

APP_NAME="dune3d"
APP_BUNDLE="${OUTPUT_DIR}/${APP_NAME}.app"

ARCH_LABEL="${ARCH}"
[[ "$ARCH" == "universal" ]] && ARCH_LABEL="universal (arm64+x86_64)"
DMG_NAME="dune3d-${APP_VERSION}-macos-${ARCH}.dmg"
DMG_PATH="${OUTPUT_DIR}/${DMG_NAME}"

# ─── x86_64 Homebrew setup ───────────────────────────────────────────────────

if [[ "$ARCH" == "universal" ]] || [[ "$ARCH" == "x86_64" ]] || $SETUP_X86; then

  step "x86_64 Homebrew check"

  if [[ ! -f "${BREW_X86}/bin/brew" ]]; then
    if $SETUP_X86 || [[ "$ARCH" == "universal" ]] || [[ "$ARCH" == "x86_64" ]]; then
      warn "x86_64 Homebrew not found at ${BREW_X86}"
      echo
      echo -e "${CYAN}│  ${WHITE}Installing x86_64 Homebrew under Rosetta...${RESET}"
      echo -e "${CYAN}│  ${DIM}(This is a one-time operation)${RESET}"
      echo
      arch -x86_64 /bin/bash -c \
        "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
      ok "x86_64 Homebrew installed at ${BREW_X86}"
    else
      fail "x86_64 Homebrew not found. Run with --setup-x86 to install it."
    fi
  else
    ok "x86_64 Homebrew found at ${BREW_X86}"
  fi

  X86_BREW="${BREW_X86}/bin/brew"

  # List of packages needed (mirrors setup-macos.sh)
  X86_PKGS=(
    meson ninja cmake pkg-config
    python@3.13 gobject-introspection pygobject3
    gtk4 gtkmm4 libepoxy librsvg cairo
    eigen glm opencascade
    harfbuzz freetype libpng
    ossp-uuid dylibbundler
  )

  info "Checking x86_64 packages..."
  MISSING_X86=()
  for pkg in "${X86_PKGS[@]}"; do
    arch -x86_64 "${X86_BREW}" list "$pkg" &>/dev/null || MISSING_X86+=("$pkg")
  done

  if [[ ${#MISSING_X86[@]} -gt 0 ]]; then
    warn "Missing x86_64 packages: ${MISSING_X86[*]}"
    echo -ne "\n  ${BYELLOW}Install missing x86_64 packages now? [y/N] ${RESET}"
    read -r confirm
    if [[ "$confirm" =~ ^[Yy]$ ]]; then
      for pkg in "${MISSING_X86[@]}"; do
        echo -e "${CYAN}│  ${DIM}→ arch -x86_64 brew install ${pkg}${RESET}"
        arch -x86_64 "${X86_BREW}" install "$pkg" &>/dev/null \
          && echo -e "${BGREEN}│  ✓ ${pkg}${RESET}" \
          || echo -e "${BYELLOW}│  ⚠ ${pkg} (skipped/failed)${RESET}"
      done
    else
      fail "Cannot build x86_64 without required packages."
    fi
  else
    ok "All x86_64 packages present"
  fi

  if $SETUP_X86; then
    ok "x86_64 environment is ready"
    div
    echo -e "\n  ${BGREEN}You can now run:${RESET}  ${WHITE}$0 --universal${RESET}\n"
    exit 0
  fi

  div
fi

# ─── Preflight ───────────────────────────────────────────────────────────────

step "Preflight checks"

[[ "$(uname)" == "Darwin" ]] || fail "This script is for macOS only."
ok "macOS $(sw_vers -productVersion)"

command -v brew &>/dev/null || fail "arm64 Homebrew not found"
ok "arm64 Homebrew at ${BREW_ARM}"

if ! command -v dylibbundler &>/dev/null; then
  info "Installing dylibbundler..."
  brew install dylibbundler &>/dev/null
fi
ok "dylibbundler found"

command -v glib-compile-schemas &>/dev/null || fail "glib-compile-schemas not found"
ok "glib-compile-schemas found"

command -v gdk-pixbuf-query-loaders &>/dev/null || fail "gdk-pixbuf-query-loaders not found"
ok "gdk-pixbuf-query-loaders found"

if [[ "$ARCH" == "universal" ]]; then
  command -v lipo &>/dev/null || fail "lipo not found (install Xcode Command Line Tools)"
  ok "lipo found"
fi

kv "Version"    "${APP_VERSION} \"${APP_NAME_QUOTED}\""
kv "Arch"       "${ARCH_LABEL}"
kv "Output"     "${OUTPUT_DIR}"
kv "Bundle"     "${APP_BUNDLE}"
kv "DMG"        "${DMG_NAME}"
div

# ─── Build ───────────────────────────────────────────────────────────────────

build_arch() {
  local arch="$1"         # arm64 | x86_64
  local build_dir="$2"
  local brew_pfx="$3"

  if [[ "$arch" == "arm64" ]]; then
    GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      PATH="${brew_pfx}/bin:$PATH" \
      meson setup --reconfigure "${build_dir}" &>/dev/null || \
    GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      PATH="${brew_pfx}/bin:$PATH" \
      meson setup "${build_dir}" &>/dev/null
    GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      meson compile -C "${build_dir}" 2>&1 | tail -3
  else
    # x86_64: run everything under Rosetta
    arch -x86_64 env \
      GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      PATH="${brew_pfx}/bin:/usr/bin:/bin" \
      "${brew_pfx}/bin/meson" setup --reconfigure "${build_dir}" &>/dev/null || \
    arch -x86_64 env \
      GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      PATH="${brew_pfx}/bin:/usr/bin:/bin" \
      "${brew_pfx}/bin/meson" setup "${build_dir}" &>/dev/null
    arch -x86_64 env \
      GI_TYPELIB_PATH="${brew_pfx}/lib/girepository-1.0" \
      "${brew_pfx}/bin/meson" compile -C "${build_dir}" 2>&1 | tail -3
  fi
}

if $BUILD; then
  if [[ "$ARCH" == "arm64" ]] || [[ "$ARCH" == "universal" ]]; then
    step "Building arm64 binary"
    build_arch "arm64" "${ARM_BUILD_DIR}" "${BREW_ARM}" \
      | while IFS= read -r l; do echo -e "${CYAN}│  ${DIM}${l}${RESET}"; done
    ok "arm64 build complete"
    div
  fi

  if [[ "$ARCH" == "x86_64" ]] || [[ "$ARCH" == "universal" ]]; then
    step "Building x86_64 binary (Rosetta)"
    build_arch "x86_64" "${X86_BUILD_DIR}" "${BREW_X86}" \
      | while IFS= read -r l; do echo -e "${CYAN}│  ${DIM}${l}${RESET}"; done
    ok "x86_64 build complete"
    div
  fi
fi

# Validate binaries exist
ARM_BINARY="${ARM_BUILD_DIR}/${APP_NAME}"
X86_BINARY="${X86_BUILD_DIR}/${APP_NAME}"

if [[ "$ARCH" == "arm64" ]]; then
  [[ -f "$ARM_BINARY" ]] || fail "arm64 binary not found at ${ARM_BINARY} — add --build"
  ok "arm64 binary: $(du -sh "$ARM_BINARY" | cut -f1)"
elif [[ "$ARCH" == "x86_64" ]]; then
  [[ -f "$X86_BINARY" ]] || fail "x86_64 binary not found at ${X86_BINARY} — add --build"
  ok "x86_64 binary: $(du -sh "$X86_BINARY" | cut -f1)"
else
  [[ -f "$ARM_BINARY" ]] || fail "arm64 binary not found at ${ARM_BINARY} — add --build"
  [[ -f "$X86_BINARY" ]] || fail "x86_64 binary not found at ${X86_BINARY} — add --build"
  ok "arm64 binary:  $(du -sh "$ARM_BINARY" | cut -f1)"
  ok "x86_64 binary: $(du -sh "$X86_BINARY" | cut -f1)"
fi
div

# ─── Create .app skeleton ────────────────────────────────────────────────────

step "Creating .app bundle structure"

rm -rf "${APP_BUNDLE}"
mkdir -p "${OUTPUT_DIR}"

MACOS_DIR="${APP_BUNDLE}/Contents/MacOS"
FRAMEWORKS_DIR="${APP_BUNDLE}/Contents/Frameworks"
RESOURCES_DIR="${APP_BUNDLE}/Contents/Resources"
SCHEMAS_DIR="${RESOURCES_DIR}/share/glib-2.0/schemas"
PIXBUF_LOADERS_DIR="${RESOURCES_DIR}/lib/gdk-pixbuf-2.0/2.10.0/loaders"
ICONS_DIR="${RESOURCES_DIR}/share/icons"
FONTS_DIR="${RESOURCES_DIR}/etc/fonts"

mkdir -p "${MACOS_DIR}" "${FRAMEWORKS_DIR}" "${RESOURCES_DIR}"
mkdir -p "${SCHEMAS_DIR}" "${PIXBUF_LOADERS_DIR}" "${ICONS_DIR}" "${FONTS_DIR}"

ok "Bundle skeleton created"
div

# ─── Binary: copy, lipo, bundle dylibs ───────────────────────────────────────

step "Bundling binary and dylibs"

BINARY_BUNDLED="${MACOS_DIR}/${APP_NAME}-bin"

progress "Preparing binary"
if [[ "$ARCH" == "universal" ]]; then
  lipo -create "${ARM_BINARY}" "${X86_BINARY}" -output "${BINARY_BUNDLED}"
elif [[ "$ARCH" == "x86_64" ]]; then
  cp "${X86_BINARY}" "${BINARY_BUNDLED}"
else
  cp "${ARM_BINARY}" "${BINARY_BUNDLED}"
fi
chmod +x "${BINARY_BUNDLED}"
done_

progress "Running dylibbundler (collecting arm64 dylibs)"
# dylibbundler reads the arm64 slice's load commands — covers all deps
dylibbundler \
  --overwrite-dir \
  --bundle-deps \
  --fix-file "${BINARY_BUNDLED}" \
  --dest-dir "${FRAMEWORKS_DIR}" \
  --install-path "@executable_path/../Frameworks/" \
  2>&1 | grep -v "^$" | tail -3 || true
done_

# Remove duplicate LC_RPATH (dylibbundler adds one per replaced rpath)
progress "Deduplicating + re-signing binary"
dedup_rpaths_and_sign "${BINARY_BUNDLED}"
done_

# Same treatment for all Frameworks dylibs (OpenCASCADE etc. also accumulate dupes)
progress "Deduplicating LC_RPATH in Frameworks dylibs"
for fw_lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  rpath_count="$(otool -l "${fw_lib}" 2>/dev/null | grep -c LC_RPATH || true)"
  [[ "${rpath_count}" -le 1 ]] && continue
  dedup_rpaths_and_sign "${fw_lib}"
done
done_

# ── Universal: lipo each Frameworks dylib with its x86_64 counterpart ────────

if [[ "$ARCH" == "universal" ]]; then
  progress "Creating universal Frameworks dylibs"
  lipo_ok=0; lipo_skip=0
  for arm_lib in "${FRAMEWORKS_DIR}"/*.dylib; do
    lib_name="$(basename "$arm_lib")"
    # x86 counterpart: same name but under /usr/local
    x86_lib="${BREW_X86}/lib/${lib_name}"

    # Also search opt/ symlink tree if not directly in lib/
    if [[ ! -f "$x86_lib" ]]; then
      x86_lib="$(find "${BREW_X86}/opt" -name "${lib_name}" 2>/dev/null | head -1 || true)"
    fi

    if [[ -f "$x86_lib" ]]; then
      lipo -create "$arm_lib" "$x86_lib" -output "${arm_lib}.fat" 2>/dev/null \
        && mv "${arm_lib}.fat" "$arm_lib" \
        && (( lipo_ok++ )) \
        || (( lipo_skip++ ))
    else
      (( lipo_skip++ ))
      # warn "  no x86_64 for ${lib_name}" # uncomment to debug
    fi
    # Re-fix paths in the (now fat) dylib — it has /usr/local refs from x86 slice
    fix_homebrew_deps "$arm_lib" "@executable_path/../Frameworks"
    codesign --force --sign - "$arm_lib" 2>/dev/null || true
  done
  done_
  ok "Universal dylibs: ${lipo_ok} fat, ${lipo_skip} arm64-only"
fi

LIB_COUNT="$(ls "${FRAMEWORKS_DIR}" | wc -l | tr -d ' ')"
ok "Bundled ${LIB_COUNT} dylibs into Frameworks/"
div

# ─── GDK-Pixbuf loaders ──────────────────────────────────────────────────────

step "Bundling GDK-Pixbuf loaders"

ARM_LOADERS_SRC="$(brew --prefix gdk-pixbuf)/lib/gdk-pixbuf-2.0/2.10.0/loaders"
X86_LOADERS_SRC="${BREW_X86}/lib/gdk-pixbuf-2.0/2.10.0/loaders"

# Generate cache from live (unfixed) arm64 loaders first
progress "Generating loaders.cache"
CACHE_TMP="$(mktemp)"
gdk-pixbuf-query-loaders "${ARM_LOADERS_SRC}"/*.so > "${CACHE_TMP}"
sed "s|${ARM_LOADERS_SRC}|@LOADERS_DIR@|g" "${CACHE_TMP}" \
  > "${RESOURCES_DIR}/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"
rm "${CACHE_TMP}"
done_

progress "Copying + fixing loader plugins"
for arm_so in "${ARM_LOADERS_SRC}"/*.so; do
  so_name="$(basename "$arm_so")"
  dest="${PIXBUF_LOADERS_DIR}/${so_name}"

  if [[ "$ARCH" == "universal" ]] && [[ -f "${X86_LOADERS_SRC}/${so_name}" ]]; then
    lipo -create "$arm_so" "${X86_LOADERS_SRC}/${so_name}" -output "$dest" 2>/dev/null \
      || cp "$arm_so" "$dest"
  else
    cp "$arm_so" "$dest"
  fi

  # Copy any missing deps that the loader needs
  loader_deps="$(otool -L "$dest" 2>/dev/null \
    | awk '{print $1}' | grep -E '^(/opt/homebrew|/usr/local)' || true)"
  if [[ -n "$loader_deps" ]]; then
    while IFS= read -r dep; do
      lib_name="$(basename "$dep")"
      [[ -f "${FRAMEWORKS_DIR}/${lib_name}" ]] && continue
      [[ -f "$dep" ]] && cp "$dep" "${FRAMEWORKS_DIR}/${lib_name}" || true
    done <<< "$loader_deps"
  fi

  # @loader_path/../../../../Frameworks = Contents/Frameworks from loaders/ dir
  fix_homebrew_deps "$dest" "@loader_path/../../../../Frameworks"
done
done_

# Fix any new Frameworks deps brought in by loaders
progress "Fixing transitive Frameworks deps"
for lib in "${FRAMEWORKS_DIR}"/*.dylib; do
  fix_homebrew_deps "$lib" "@executable_path/../Frameworks"
done
done_

LOADER_COUNT="$(ls "${PIXBUF_LOADERS_DIR}" | wc -l | tr -d ' ')"
ok "Bundled ${LOADER_COUNT} pixbuf loaders"
div

# ─── GLib schemas ────────────────────────────────────────────────────────────

step "Bundling GLib schemas"

progress "Copying + compiling schemas"
find "${BREW_ARM}/share/glib-2.0/schemas" -name "*.gschema.xml" \
  -exec cp {} "${SCHEMAS_DIR}/" \;
glib-compile-schemas "${SCHEMAS_DIR}"
done_

SCHEMA_COUNT="$(ls "${SCHEMAS_DIR}"/*.gschema.xml 2>/dev/null | wc -l | tr -d ' ')"
ok "Compiled ${SCHEMA_COUNT} schemas"
div

# ─── Icons ───────────────────────────────────────────────────────────────────

step "Bundling icons"

progress "Copying .icns"
cp "${REPO_DIR}/src/icons/dune3d.icns" "${RESOURCES_DIR}/dune3d.icns"
done_

HICOLOR_SRC="${BREW_ARM}/share/icons/hicolor"
if [[ -d "$HICOLOR_SRC" ]]; then
  progress "Copying hicolor theme"
  cp -R "${HICOLOR_SRC}" "${ICONS_DIR}/hicolor"
  done_
fi

progress "Copying app action icons"
mkdir -p "${ICONS_DIR}/hicolor"
cp -R "${REPO_DIR}/src/icons/scalable" "${ICONS_DIR}/hicolor/scalable" 2>/dev/null || true
done_

progress "Building icon cache"
command -v gtk4-update-icon-cache &>/dev/null \
  && gtk4-update-icon-cache -f -t "${ICONS_DIR}/hicolor" 2>/dev/null || true
done_

ok "Icons bundled"
div

# ─── Fontconfig ──────────────────────────────────────────────────────────────

step "Bundling fontconfig"

cat > "${FONTS_DIR}/fonts.conf" << 'EOF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "fonts.dtd">
<fontconfig>
  <dir>/System/Library/Fonts</dir>
  <dir>/Library/Fonts</dir>
  <dir>~/Library/Fonts</dir>
  <dir>/System/Library/AssetsV2/com_apple_MobileAsset_Font7</dir>
  <cachedir>~/.cache/fontconfig</cachedir>
  <config><rescan><int>30</int></rescan></config>
</fontconfig>
EOF

ok "fonts.conf written (uses system fonts)"
div

# ─── Launcher script ─────────────────────────────────────────────────────────

step "Creating launcher script"

cat > "${MACOS_DIR}/${APP_NAME}" << 'LAUNCHER'
#!/usr/bin/env bash
# dune3d macOS launcher — sets up GTK runtime environment

SCRIPT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESOURCES="$SCRIPT/../Resources"

LOADERS_DIR="$RESOURCES/lib/gdk-pixbuf-2.0/2.10.0/loaders"
CACHE_FILE="$RESOURCES/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache"

# Patch loaders.cache placeholder on first run (cache is write-once)
if [[ -f "$CACHE_FILE" ]] && grep -q '@LOADERS_DIR@' "$CACHE_FILE"; then
  sed -i '' "s|@LOADERS_DIR@|${LOADERS_DIR}|g" "$CACHE_FILE"
fi

export GDK_PIXBUF_MODULE_FILE="$CACHE_FILE"
export GSETTINGS_SCHEMA_DIR="$RESOURCES/share/glib-2.0/schemas"
export XDG_DATA_DIRS="$RESOURCES/share:${XDG_DATA_DIRS:-/usr/local/share:/usr/share}"
export FONTCONFIG_FILE="$RESOURCES/etc/fonts/fonts.conf"
export GTK_DATA_PREFIX="$RESOURCES"

exec "$SCRIPT/dune3d-bin" "$@"
LAUNCHER

chmod +x "${MACOS_DIR}/${APP_NAME}"
ok "Launcher script created"
div

# ─── Info.plist ──────────────────────────────────────────────────────────────

step "Writing Info.plist"

cat > "${APP_BUNDLE}/Contents/Info.plist" << PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>CFBundleName</key>           <string>dune3d</string>
  <key>CFBundleDisplayName</key>    <string>Dune 3D</string>
  <key>CFBundleIdentifier</key>     <string>org.dune3d.dune3d</string>
  <key>CFBundleVersion</key>        <string>${APP_VERSION}</string>
  <key>CFBundleShortVersionString</key> <string>${APP_VERSION}</string>
  <key>CFBundleExecutable</key>     <string>dune3d</string>
  <key>CFBundleIconFile</key>       <string>dune3d</string>
  <key>CFBundlePackageType</key>    <string>APPL</string>
  <key>NSHighResolutionCapable</key>    <true/>
  <key>NSRequiresAquaSystemAppearance</key> <false/>
  <key>CFBundleDocumentTypes</key>
  <array>
    <dict>
      <key>CFBundleTypeExtensions</key>
      <array><string>d3d</string></array>
      <key>CFBundleTypeName</key>  <string>Dune 3D Document</string>
      <key>CFBundleTypeRole</key>  <string>Editor</string>
    </dict>
  </array>
</dict>
</plist>
PLIST

ok "Info.plist written"
div

# ─── Code signing ────────────────────────────────────────────────────────────

if [[ -n "$SIGN_ID" ]]; then
  step "Code signing with ${SIGN_ID}"
  progress "Signing Frameworks"
  find "${FRAMEWORKS_DIR}" -name "*.dylib" \
    -exec codesign --force --sign "$SIGN_ID" {} \; 2>/dev/null || true
  done_
  progress "Signing pixbuf loaders"
  find "${PIXBUF_LOADERS_DIR}" -name "*.so" \
    -exec codesign --force --sign "$SIGN_ID" {} \; 2>/dev/null || true
  done_
  progress "Signing app bundle"
  codesign --force --deep --sign "$SIGN_ID" "${APP_BUNDLE}"
  done_
  ok "Signed with: ${SIGN_ID}"
  div
fi

# ─── Verify fat binary (universal mode) ──────────────────────────────────────

if [[ "$ARCH" == "universal" ]]; then
  step "Verifying universal binary"
  ARCHES="$(lipo -archs "${BINARY_BUNDLED}" 2>/dev/null)"
  kv "Binary arches" "${ARCHES}"
  [[ "$ARCHES" == *"x86_64"* ]] && [[ "$ARCHES" == *"arm64"* ]] \
    && ok "Both arm64 and x86_64 slices present" \
    || warn "Expected both arm64 and x86_64 — got: ${ARCHES}"
  FAT_LIBS="$(for f in "${FRAMEWORKS_DIR}"/*.dylib; do
    a="$(lipo -archs "$f" 2>/dev/null)"; [[ "$a" == *x86_64* ]] && echo "$(basename $f)"; done | wc -l | tr -d ' ')"
  kv "Fat Frameworks" "${FAT_LIBS} / ${LIB_COUNT}"
  div
fi

# ─── Create DMG ──────────────────────────────────────────────────────────────

step "Creating DMG"

[[ -f "${DMG_PATH}" ]] && rm -f "${DMG_PATH}"

DMG_TMP="$(mktemp -d)"
DMG_STAGE="${DMG_TMP}/dmg-stage"
mkdir -p "${DMG_STAGE}"

progress "Staging bundle"
cp -R "${APP_BUNDLE}" "${DMG_STAGE}/"
ln -s /Applications "${DMG_STAGE}/Applications"
done_

progress "Creating compressed DMG"
DMG_SIZE_KB="$(du -sk "${DMG_STAGE}" | cut -f1)"
DMG_SIZE_MB=$(( (DMG_SIZE_KB / 1024) + 50 ))
hdiutil create \
  -srcfolder "${DMG_STAGE}" \
  -volname "Dune 3D ${APP_VERSION}" \
  -fs HFS+ -fsargs "-c c=64,a=16,b=16" \
  -format UDRW -size "${DMG_SIZE_MB}m" \
  "${DMG_TMP}/rw.dmg" &>/dev/null
hdiutil convert "${DMG_TMP}/rw.dmg" \
  -format UDZO -imagekey zlib-level=9 \
  -o "${DMG_PATH}" &>/dev/null
rm -rf "${DMG_TMP}"
done_

DMG_SIZE="$(du -sh "${DMG_PATH}" | cut -f1)"
ok "DMG created: ${DMG_NAME} (${DMG_SIZE})"
div

# ─── Summary ─────────────────────────────────────────────────────────────────

step "Done!"
echo
echo -e "  ${BGREEN}★${RESET}  ${BWHITE}dune3d ${APP_VERSION} \"${APP_NAME_QUOTED}\" [${ARCH_LABEL}]${RESET}  ${BGREEN}★${RESET}"
echo
kv "App bundle" "${APP_BUNDLE}"
kv "DMG"        "${DMG_PATH}"
kv "DMG size"   "${DMG_SIZE}"
echo
echo -e "${CYAN}│${RESET}  ${DIM}open ${APP_BUNDLE}${RESET}"
echo -e "${CYAN}│${RESET}  ${DIM}open ${DMG_PATH}${RESET}"
echo
div
echo
