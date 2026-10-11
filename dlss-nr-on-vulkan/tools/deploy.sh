#!/usr/bin/env bash
# ============================================================================
#  deploy.sh  -  run DLSS-NR on Intel against a game (Linux / macOS)
#
#  One script for the whole setup: weights, build, install.
#
#    1. weights  - runs scripts/get_weights.py against a DLL you supply (--dll), which
#                  clones MLX-DLSS at its pinned commit and writes
#                  work/mlxw/dlssnr-logical.safetensors. Skipped with --skip-weights,
#                  or when the file is already there.
#    2. build    - make builds libxmx.so, libnr_layer.so, libnr_image.so and the shaders.
#    3. install  - copies the layer into the game folder next to a manifest whose
#                  library_path is ./ and its name, copies the runtime the daemon needs,
#                  and writes a launcher.
#
#  What it deliberately does not do:
#
#    * It does NOT copy the model weights anywhere. The launcher points the daemon at
#      this checkout with NR_ROOT, so the weights stay in work/mlxw/ and nothing large
#      can be passed on by accident with the game folder.
#    * It DOES copy the runtime the daemon needs to start: work/mlx-dlss (the MLX
#      extractor the daemon imports), work/libxmx.so, work/libnr_image.so and
#      work/*.spv. Without them the daemon stops at start.
#
#  Why the manifest matters: the Vulkan loader finds a layer through its manifest
#  plus VK_LAYER_PATH, and the manifest names the library as ./NAME. A path is taken
#  relative to the manifest's folder; a bare file name would be handed to dlopen, which
#  searches the system's library path and not that folder, and the layer would not load.
#
#  Usage:
#    ./deploy.sh --game /path/to/Game --dll /path/to/nvngx_dlssnr.dll
#                [--name libnr_layer.so] [--exe /path/to/Game/game]
#                [--skip-weights] [--skip-build]
#    ./deploy.sh --release dist/dlss-nr-linux [--skip-weights] [--skip-build]
# ============================================================================
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORK="$REPO/work"
NR_DEPLOY_PYTHON="${NR_PYTHON:-python3}"
GAME=""
DLL=""
NAME="libnr_layer.so"
EXE=""
RELEASE=""
SKIP_WEIGHTS=0
SKIP_BUILD=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --game)         GAME="$2"; shift 2 ;;
    --dll)          DLL="$2";  shift 2 ;;
    --name)         NAME="$2"; shift 2 ;;
    --exe)          EXE="$2";  shift 2 ;;
    --release)      RELEASE="$2"; shift 2 ;;
    --skip-weights) SKIP_WEIGHTS=1; shift ;;
    --skip-build)   SKIP_BUILD=1; shift ;;
    *) echo "Unknown argument: $1" >&2; exit 2 ;;
  esac
done

# --release assembles a folder for other people and installs into nobody's game, so it is
# the one mode that does not need --game.
if [[ -z "$RELEASE" ]]; then
  [[ -n "$GAME" ]] || { echo "ERROR: --game is required (or --release <folder>)" >&2; exit 2; }
  [[ -d "$GAME" ]] || { echo "ERROR: game directory not found: $GAME" >&2; exit 2; }
fi

# ---- weights, delegated to scripts/get_weights.py ----
# It clones MLX-DLSS at the pinned commit, runs its extractor against your DLL and
# checks that 649 tensors came out, so there is one definition of "the weights are
# present and correct" rather than a second copy of the two commands here.
if [[ $SKIP_WEIGHTS -eq 1 ]]; then
  echo "[skip] weights (--skip-weights)"
elif [[ -f "$WORK/mlxw/dlssnr-logical.safetensors" ]]; then
  echo "[1/3] weights already at $WORK/mlxw/dlssnr-logical.safetensors"
else
  [[ -n "$DLL" ]] || {
    echo "ERROR: no weights yet and no --dll given." >&2
    echo "       Pass --dll /path/to/nvngx_dlssnr.dll, or --skip-weights if you have them." >&2
    exit 2
  }
  [[ -f "$DLL" ]] || { echo "ERROR: DLL not found: $DLL" >&2; exit 2; }
  echo "[1/3] extracting weights with scripts/get_weights.py ..."
  "$NR_DEPLOY_PYTHON" "$REPO/scripts/get_weights.py" "$DLL"
fi

# ---- build ----
if [[ $SKIP_BUILD -eq 1 ]]; then
  echo "[skip] build (--skip-build)"
else
  echo "[2/3] building the layer ..."
  ( cd "$REPO" && make )
  if [[ -n "$RELEASE" ]]; then
    # The release's CPU floor, whatever NR_IMAGE_ARCH this shell carries. It is also the
    # default, and make rebuilds the library whenever its flags change.
    ( cd "$REPO" && make work/libnr_image.so NR_IMAGE_ARCH=-march=x86-64-v3 )
  fi
fi
[[ -f "$WORK/libnr_layer.so" ]] || { echo "ERROR: $WORK/libnr_layer.so missing; build it first" >&2; exit 3; }

# ---- checked release assembly, or installation into a game ----
if [[ -n "$RELEASE" ]]; then
  "$NR_DEPLOY_PYTHON" "$REPO/scripts/build_release.py" "$RELEASE" --platform linux
  exit 0
fi

# ---- install into the game folder ----
echo "[3/3] installing into $GAME ..."
DEPLOY="$GAME/dlss-nr"
mkdir -p "$DEPLOY/src" "$DEPLOY/work"

cp "$WORK/libnr_layer.so" "$DEPLOY/$NAME"
echo "  layer:      $DEPLOY/$NAME"

MANIFEST_SRC="$REPO/src/layer/VkLayer_dlss_nr.json"
MANIFEST_DST="$DEPLOY/VkLayer_dlss_nr.json"
[[ -f "$MANIFEST_SRC" ]] || { echo "ERROR: manifest template not found: $MANIFEST_SRC" >&2; exit 3; }
sed "s#LIBRARY_PATH_PLACEHOLDER#./$NAME#g" "$MANIFEST_SRC" > "$MANIFEST_DST"
echo "  manifest:   $MANIFEST_DST  library_path = ./$NAME"

for d in layer ref gpu bench; do
  [[ -d "$REPO/src/$d" ]] && cp -r "$REPO/src/$d" "$DEPLOY/src/$d"
done

# The runtime the daemon imports. The weights are NOT copied.
if [[ -d "$WORK/mlx-dlss" ]]; then
  cp -r "$WORK/mlx-dlss" "$DEPLOY/work/mlx-dlss"
  echo "  runtime:    work/mlx-dlss"
else
  echo "  WARNING: $WORK/mlx-dlss is missing; the daemon needs it (see README)"
fi
for f in "$WORK"/libxmx.so* "$WORK"/libnr_image.so* "$WORK"/*.spv; do
  [[ -e "$f" ]] && cp "$f" "$DEPLOY/work/" || true
done
echo "  runtime:    libxmx, libnr_image, shaders"

find "$DEPLOY/src" -type d -name __pycache__ -prune -exec rm -rf {} + 2>/dev/null

# ---- launcher ----
LAUNCH="$DEPLOY/launch-nr.sh"
{
  echo '#!/usr/bin/env bash'
  echo 'set -e'
  echo '# The loader finds the layer through VK_LAYER_PATH plus the manifest.'
  echo "export VK_LAYER_PATH=\"$DEPLOY\""
  echo 'export VK_INSTANCE_LAYERS=VK_LAYER_dlssnr_intel'
  echo 'export ENABLE_NR_LAYER=1'
  echo 'export NR_LAYER_SPAWN=1'
  echo 'export NR_LAYER_LIVE=1'
  echo 'export NR_LAYER_SOCKET="${NR_LAYER_SOCKET:-/tmp/nr_layer.sock}"'
  echo '# The weights stay in the checkout; only this path points at them.'
  echo "export NR_ROOT=\"$REPO\""
  echo ''
  if [[ -n "$EXE" ]]; then
    echo "exec \"$EXE\" \"\$@\""
  else
    echo 'GAME_EXE="${GAME_EXE:-}"'
    echo 'if [[ -z "$GAME_EXE" ]]; then echo "Set GAME_EXE in this file" >&2; exit 1; fi'
    echo 'exec "$GAME_EXE" "$@"'
  fi
} > "$LAUNCH"
chmod +x "$LAUNCH"
echo "  launcher:   $LAUNCH"

echo
echo "Done. Install folder: $DEPLOY"
echo "  $NAME, VkLayer_dlss_nr.json, src/, work/ (runtime only - no weights)"
echo
echo "Weights stay in $WORK/mlxw; the launcher points NR_ROOT at the checkout."
echo "Run the game through $LAUNCH so VK_LAYER_PATH is set."
echo
echo "COMPLIANCE: nothing NVIDIA's is shipped. The weights come from a DLL you own and"
echo "are never copied into the game folder."
