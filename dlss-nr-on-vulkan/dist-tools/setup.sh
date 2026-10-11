#!/usr/bin/env bash
# ============================================================================
#  setup.sh  -  configure a DLSS-NR release to run a game (Linux / macOS)
#
#  Ships inside the release folder, next to the built layer and the runtime. It is the
#  last step a user runs; nothing here compiles anything.
#
#    1. Find nvngx_dlssnr.dll. Three ways, in the order offered:
#         a) one sitting next to this script
#         b) a folder you pick, searched for the file
#            (no download: the DLL is NVIDIA's, and this script will not fetch it)
#    2. Extract the logical weights from that DLL (needs Python 3 with NumPy and
#       safetensors).
#    3. Install the layer: copy it under a name you choose, write the manifest so its
#       library_path is that file, and write a launcher pointing the daemon at this
#       folder.
#
#  Usage:
#    ./setup.sh --game /path/to/Game
#    ./setup.sh --game ... --dll /path/to/nvngx_dlssnr.dll
#    ./setup.sh --game ... --name libnr_layer.so --skip-weights
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME=""
DLL=""
NAME="libnr_layer.so"
SKIP_WEIGHTS=0
DRY_RUN=0
NR_SETUP_PYTHON="${NR_PYTHON:-python3}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --game)         GAME="$2"; shift 2 ;;
    --dll)          DLL="$2";  shift 2 ;;
    --name)         NAME="$2"; shift 2 ;;
        --skip-weights) SKIP_WEIGHTS=1; shift ;;
--dry-run)      DRY_RUN=1; shift ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
done

echo "============================================================"
echo " DLSS-NR on Intel - setup"
echo "============================================================"
echo

# ---- what is in this folder ----
if [[ ! -f "$HERE/nr_layer.so" ]]; then
  echo "ERROR: nr_layer.so is not in $HERE" >&2
  echo "       Run this from inside the release folder it came in." >&2
  exit 1
fi
if [[ ! -d "$HERE/work" ]]; then
  echo "ERROR: the work/ folder is missing next to nr_layer.so" >&2
  exit 1
fi
echo "Release folder: $HERE"
echo

weights="$HERE/work/mlxw/dlssnr-logical.safetensors"

# A path the user gave — by --dll or interactively — becomes the DLL itself, or fails.
# Both entry points go through here so they cannot disagree about what is valid.
resolve_dll() {
  local candidate="$1" name
  candidate="${candidate%\"}"; candidate="${candidate#\"}"
  candidate="${candidate%/}"
  [[ -n "$candidate" ]] || return 1
  if [[ -f "$candidate/nvngx_dlssnr.dll" ]]; then
    DLL="$candidate/nvngx_dlssnr.dll"; return 0
  fi
  if [[ -d "$candidate" ]]; then
    echo "  That folder does not contain nvngx_dlssnr.dll."
    return 1
  fi
  if [[ -f "$candidate" ]]; then
    name="$(basename "$candidate")"
    if [[ "$name" != "nvngx_dlssnr.dll" ]]; then
      echo "  That file is $name, not nvngx_dlssnr.dll."
      return 1
    fi
    DLL="$candidate"; return 0
  fi
  echo "  $candidate does not exist."
  return 1
}

# --skip-weights means "the weights are already here", so check rather than trust: a
# release folder with no weights that still installs fails later, in the game, with
# nothing on screen pointing back to here.
if [[ $SKIP_WEIGHTS -eq 1 ]]; then
  if [[ -f "$weights" ]]; then
    echo "[1/4] weights present; --skip-weights trusts that"
    SKIP_WEIGHTS=2                      # 2 = verified, skip the extraction below
  else
    echo "ERROR: --skip-weights was given but the weights are not here." >&2
    echo "       Expected: $weights" >&2
    echo "       Drop --skip-weights to extract them, passing --dll <path> if needed." >&2
    exit 3
  fi
fi

if [[ $SKIP_WEIGHTS -eq 0 ]]; then
  # ---- 1. the NVIDIA DLL ----
  if [[ -n "$DLL" ]] && ! resolve_dll "$DLL"; then
    echo "ERROR: --dll did not name a usable nvngx_dlssnr.dll." >&2
    exit 3
  fi
  if [[ -n "$DLL" ]]; then
    echo "[1/4] DLL from the command line: $DLL"
  fi
  if [[ -z "$DLL" && -f "$HERE/nvngx_dlssnr.dll" ]]; then
    DLL="$HERE/nvngx_dlssnr.dll"
    echo "[1/4] found nvngx_dlssnr.dll next to this script"
  fi
  if [[ -z "$DLL" ]]; then
    echo "[1/4] no nvngx_dlssnr.dll beside this script."
    echo
    echo "      Where is yours? Pass a path to the DLL or to the folder holding it,"
    echo "      or press Enter to stop here."
    echo
    read -r -p "Path: " ANSWER || ANSWER=""
    if [[ -n "$ANSWER" ]]; then
      if resolve_dll "$ANSWER"; then
        echo "  using $DLL"
      fi
    fi
  fi

  # ---- 1c. nothing to fetch ----
  # The DLL is NVIDIA's. Every project in this space requires the user to supply it, and
  # this one does too: fetching it would mean redistributing it, which is why there is no
  # download here.
  if [[ -z "$DLL" ]]; then
    echo >&2
    echo "  nvngx_dlssnr.dll was not found." >&2
    echo >&2
    echo "  Put it beside this script, or pass --dll <path>, or point at a folder that" >&2
    echo "  holds it. The file is around 158 MB." >&2
    exit 3
  fi

  # ---- 2. Python ----
  command -v "$NR_SETUP_PYTHON" >/dev/null || { echo "ERROR: Python 3 not found: $NR_SETUP_PYTHON" >&2; exit 3; }
  if ! "$NR_SETUP_PYTHON" -c "import numpy, safetensors" >/dev/null 2>&1; then
    echo "ERROR: Python is missing the packages the extractor needs." >&2
    echo "       Run:  python3 -m pip install numpy safetensors" >&2
    exit 3
  fi
  echo "[2/4] extracting weights from $DLL"
  echo "      (a few minutes; the DLL is read, nothing is written back to it)"
  "$NR_SETUP_PYTHON" "$HERE/scripts/get_weights.py" "$DLL" --work-dir "$HERE/work"
  [[ -f "$weights" ]] || { echo "ERROR: extraction reported success but $weights is missing." >&2; exit 3; }
  echo
fi

# ---- 3. the game folder ----
if [[ -z "$GAME" ]]; then
  echo "[3/4] Which game? Pass the folder holding its executable."
  read -r -p "Game folder: " GAME || GAME=""
  GAME="${GAME%\"}"; GAME="${GAME#\"}"
fi
[[ -n "$GAME" ]] || { echo "ERROR: no game folder given." >&2; exit 2; }
[[ -d "$GAME" ]] || { echo "ERROR: game folder not found: $GAME" >&2; exit 2; }

# ---- 4. install ----
# Everything above reads; this is the first thing that writes into the game folder, so
# --dry-run stops here and says what it would have done.
if [[ ${DRY_RUN:-0} -eq 1 ]]; then
  echo
  echo "[dry run] nothing was written"
  echo "  would install $HERE/nr_layer.so to $GAME/dlss-nr/$NAME"
  echo "  would write $GAME/dlss-nr/VkLayer_dlss_nr.json with library_path ./$NAME"
  echo "  would copy the runtime: work/mlx-dlss, work/*.spv, work/*.so"
  if [[ -f "$weights" ]]; then
    echo "  weights: present, and are NOT copied - the layer points at this folder"
  else
    echo "  weights: MISSING - a real run would have failed before installing"
  fi
  echo
  exit 0
fi

echo "[4/4] installing into $GAME"
DEST="$GAME/dlss-nr"
mkdir -p "$DEST" "$DEST/work" "$DEST/src"
cp "$HERE/nr_layer.so" "$DEST/$NAME"
echo "  layer:      $DEST/$NAME"

MANIFEST="$HERE/VkLayer_dlss_nr.json"
[[ -f "$MANIFEST" ]] || MANIFEST="$HERE/src/layer/VkLayer_dlss_nr.json"
[[ -f "$MANIFEST" ]] || { echo "ERROR: no VkLayer_dlss_nr.json in the release." >&2; exit 3; }
# `./NAME`, not an absolute path and not a bare name. A bare name goes to dlopen, which
# searches the system library path rather than the manifest's own folder, and the layer
# then fails to load with `create instance: -6` (6083140, measured on Linux). An absolute
# path also works, but `./NAME` is what tools/deploy.sh installs with and what was verified
# there, so both scripts write the same thing for the same platform.
sed "s|LIBRARY_PATH_PLACEHOLDER|./$NAME|g" "$MANIFEST" > "$DEST/VkLayer_dlss_nr.json"
echo "  manifest:   $DEST/VkLayer_dlss_nr.json  library_path = ./$NAME"

for d in layer ref gpu bench; do
  [[ -d "$HERE/src/$d" ]] && cp -r "$HERE/src/$d" "$DEST/src/$d"
done
[[ -d "$HERE/work/mlx-dlss" ]] && cp -r "$HERE/work/mlx-dlss" "$DEST/work/mlx-dlss"
cp "$HERE"/work/*.spv "$DEST/work/" 2>/dev/null || true
cp "$HERE"/work/*.so  "$DEST/work/" 2>/dev/null || true
find "$DEST/src" -type d -name __pycache__ -prune -exec rm -rf {} + 2>/dev/null || true
echo "  runtime:    shaders and libraries"

LAUNCH="$DEST/launch-nr.sh"
{
  echo '#!/usr/bin/env bash'
  echo 'set -e'
  echo '# The loader finds the layer through VK_LAYER_PATH plus the manifest.'
  printf 'export VK_LAYER_PATH=%q\n' "$DEST"
  echo 'export VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel'
  echo 'export ENABLE_NR_LAYER=1'
  echo 'export NR_LAYER_SPAWN=1'
  echo 'export NR_LAYER_LIVE=1'
  echo 'export NR_LAYER_SOCKET="${NR_LAYER_SOCKET:-/tmp/nr_layer.sock}"'
  echo '# The weights stay in the release folder.'
  printf 'export NR_ROOT=%q\n' "$HERE"
  printf 'export NR_SETTINGS=%q\n' "$HERE/work/nr_settings.json"
  printf 'export NR_LAYER_LOG=%q\n' "$HERE/work/nr_daemon.log"
  printf 'export NR_LAYER_TRIGGER=%q\n' "$HERE/work/nr_trigger"
  echo ''
  echo 'GAME_EXE="${GAME_EXE:-}"'
  echo 'if [[ -z "$GAME_EXE" ]]; then'
  printf '  GAME_EXE=$(find %q -maxdepth 1 -type f -perm -u+x ! -name "*.so" ! -name "*.dll" | head -1)\n' "$GAME"
  echo 'fi'
  echo 'if [[ -z "$GAME_EXE" ]]; then echo "Set GAME_EXE in this file" >&2; exit 1; fi'
  echo 'exec "$GAME_EXE" "$@"'
} > "$LAUNCH"
chmod +x "$LAUNCH"
echo "  launcher:   $LAUNCH"

echo
echo "Done."
echo "  Run the game through $LAUNCH"
echo "  Weights stayed in $HERE/work/mlxw; the launcher points NR_ROOT there."
echo
echo "COMPLIANCE: nothing NVIDIA's is redistributed by this script. The DLL it read is"
echo "yours, the weights came from it, and both stay on this machine."
