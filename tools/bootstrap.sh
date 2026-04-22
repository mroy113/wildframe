#!/bin/bash
# tools/bootstrap.sh — S0-15: Wildframe developer environment bootstrap.
#
# Takes a clean macOS machine to "first build passing" with one command.
# Idempotent: safe to re-run. Installs only what is missing.
#
# Usage:
#   tools/bootstrap.sh
#
# What it does, in order:
#   1. Verifies the host is macOS.
#   2. Ensures Xcode Command Line Tools are installed.
#   3. Installs Homebrew if missing.
#   4. Installs the Homebrew formulae Wildframe needs (cmake, ninja, llvm,
#      pkg-config, and the autotools set required for vcpkg's transitive
#      meson/autotools ports).
#   5. Gates the resulting CMake on the handoff §3 minimum (>= 3.24).
#   6. Clones vcpkg (default destination $HOME/vcpkg; honors VCPKG_ROOT if
#      already set), pins it to the builtin-baseline SHA in vcpkg.json, and
#      runs bootstrap-vcpkg.sh.
#   7. Runs the first `cmake --preset debug` + `cmake --build --preset debug`.
#
# It does NOT modify the user's shell profile. The final message prints the
# one-line VCPKG_ROOT export the developer should add themselves.
#
# Long-form rationale for each step lives in docs/DEV_SETUP.md.

set -euo pipefail

# ---------------------------------------------------------------------------
# Resolved paths and constants.
# ---------------------------------------------------------------------------

: "${VCPKG_ROOT:=$HOME/vcpkg}"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Homebrew formulae required for a cold first build.
#
# Why each one:
#   cmake, ninja        — CMakePresets.json hard-codes the Ninja generator.
#   llvm                — clang-tidy and clang-format for the NFR-7 / NFR-8
#                         gates. macOS CLT does not ship them; llvm is keg-only.
#                         cmake/ClangTooling.cmake hints both the Intel and
#                         Apple Silicon keg prefixes.
#   pkg-config          — required by vcpkg's meson-based ports (e.g. `inih`
#                         via `exiv2`) when building from a cold binary cache.
#                         Missing pkg-config makes the first configure fail.
#   autoconf*, automake, libtool
#                       — same story for vcpkg's autotools-based ports. The
#                         CI workflow installs the same set in
#                         .github/workflows/ci.yml.
BREW_PACKAGES=(
    cmake
    ninja
    llvm
    pkg-config
    autoconf
    autoconf-archive
    automake
    libtool
)

if [[ -t 1 ]]; then
    C_BOLD=$'\033[1m'
    C_DIM=$'\033[2m'
    C_RESET=$'\033[0m'
else
    C_BOLD=''
    C_DIM=''
    C_RESET=''
fi

log()  { printf '%s==>%s %s\n' "$C_BOLD" "$C_RESET" "$*"; }
note() { printf '%s    %s%s\n' "$C_DIM" "$*" "$C_RESET"; }
fail() { printf 'error: %s\n' "$*" >&2; exit 1; }

# ---------------------------------------------------------------------------
# 0. Preflight.
# ---------------------------------------------------------------------------

if [[ "$(uname -s)" != "Darwin" ]]; then
    fail "Wildframe currently supports macOS only. See docs/DEV_SETUP.md §1 and CLAUDE.md §6."
fi

# ---------------------------------------------------------------------------
# 1. Xcode Command Line Tools.
# ---------------------------------------------------------------------------

log "Checking Xcode Command Line Tools"
if ! xcode-select -p >/dev/null 2>&1; then
    note "Not installed. Launching the GUI installer; re-run this script when it finishes."
    xcode-select --install || true
    fail "Xcode CLT install started. Re-run tools/bootstrap.sh after the GUI installer completes."
fi
note "Xcode CLT present at $(xcode-select -p)."

# ---------------------------------------------------------------------------
# 2. Homebrew.
# ---------------------------------------------------------------------------

log "Checking Homebrew"
if ! command -v brew >/dev/null 2>&1; then
    note "Not installed. Running the official installer."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    if [[ -x /opt/homebrew/bin/brew ]]; then
        eval "$(/opt/homebrew/bin/brew shellenv)"
    elif [[ -x /usr/local/bin/brew ]]; then
        eval "$(/usr/local/bin/brew shellenv)"
    else
        fail "Homebrew installed but 'brew' is not on PATH. See https://brew.sh and re-run."
    fi
fi
BREW="$(command -v brew)"
note "brew: $BREW ($("$BREW" --version | head -n1))"

# ---------------------------------------------------------------------------
# 3. Homebrew formulae.
# ---------------------------------------------------------------------------

log "Ensuring Homebrew formulae"
to_install=()
for pkg in "${BREW_PACKAGES[@]}"; do
    if "$BREW" list --formula "$pkg" >/dev/null 2>&1; then
        note "present: $pkg"
    else
        to_install+=("$pkg")
    fi
done
if (( ${#to_install[@]} > 0 )); then
    note "installing: ${to_install[*]}"
    "$BREW" install "${to_install[@]}"
fi

# ---------------------------------------------------------------------------
# 4. CMake version gate.
# ---------------------------------------------------------------------------

log "Verifying CMake version"
cmake_version="$(cmake --version | head -n1 | awk '{print $3}')"
cmake_major="${cmake_version%%.*}"
cmake_rest="${cmake_version#*.}"
cmake_minor="${cmake_rest%%.*}"
if (( cmake_major < 3 )) || { (( cmake_major == 3 )) && (( cmake_minor < 24 )); }; then
    fail "CMake >= 3.24 required; detected $cmake_version. Upgrade with 'brew upgrade cmake'."
fi
note "cmake $cmake_version satisfies >= 3.24."

# ---------------------------------------------------------------------------
# 5. vcpkg clone + pin + bootstrap.
# ---------------------------------------------------------------------------

log "Setting up vcpkg at $VCPKG_ROOT"

if [[ ! -d "$VCPKG_ROOT/.git" ]]; then
    if [[ -e "$VCPKG_ROOT" ]]; then
        fail "$VCPKG_ROOT exists but is not a git checkout. Resolve manually, then re-run."
    fi
    note "Cloning microsoft/vcpkg ..."
    git clone https://github.com/microsoft/vcpkg "$VCPKG_ROOT"
else
    note "vcpkg clone already present."
    if [[ -n "$(git -C "$VCPKG_ROOT" status --porcelain)" ]]; then
        fail "vcpkg checkout at $VCPKG_ROOT has uncommitted changes. Commit or stash, then re-run."
    fi
    note "fetching latest refs ..."
    git -C "$VCPKG_ROOT" fetch --quiet origin
fi

# Pin to the builtin-baseline SHA from vcpkg.json. The regex grabs the 40-char
# hex in quotes on the builtin-baseline line; if vcpkg.json's format changes
# in a way that breaks this grep, the fail path points the reader at the file.
baseline_sha="$(
    grep -E '"builtin-baseline"' "$REPO_ROOT/vcpkg.json" \
        | sed -E 's/.*"([0-9a-f]{40})".*/\1/' \
        | head -n1
)"
if [[ ! "$baseline_sha" =~ ^[0-9a-f]{40}$ ]]; then
    fail "Could not parse builtin-baseline SHA from $REPO_ROOT/vcpkg.json."
fi
note "pinning vcpkg to builtin-baseline $baseline_sha"
git -C "$VCPKG_ROOT" checkout --quiet "$baseline_sha"

note "running bootstrap-vcpkg.sh"
"$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

# ---------------------------------------------------------------------------
# 6. First configure + build.
# ---------------------------------------------------------------------------

log "Running 'cmake --preset debug' (first configure — vcpkg will build Qt 6,"
log "OpenCV, ONNX Runtime, Exiv2, and LibRaw from source; expect hours on a"
log "cold cache)"
export VCPKG_ROOT
cd "$REPO_ROOT"
cmake --preset debug

log "Running 'cmake --build --preset debug'"
cmake --build --preset debug

# ---------------------------------------------------------------------------
# 7. Next-steps message.
# ---------------------------------------------------------------------------

log "Bootstrap complete."
cat <<EOF

${C_BOLD}Next steps${C_RESET}

Add the following line to your shell profile (~/.zshrc for zsh, which is the
macOS default; ~/.bash_profile for bash) so subsequent 'cmake --preset ...'
calls find the toolchain without re-exporting VCPKG_ROOT every time:

    export VCPKG_ROOT="$VCPKG_ROOT"

Then verify the build stays green:

    cmake --build --preset debug
    ctest --preset debug

See docs/DEV_SETUP.md for the long-form walkthrough, per-tool rationale, and
what this script does not yet automate.
EOF
