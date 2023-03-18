#!/bin/sh
# usage: build-presets

SELF="$0"
SELF_DIR=$(cd $(dirname "${SELF}") && pwd -P)
SELF_DIR="${SELF_DIR:-$(pwd)}"
LIBHB_DIR="${SELF_DIR}/../libhb"
PRESETS_DIR="${SELF_DIR}/../preset"

if ! cd "${PRESETS_DIR}"; then
    echo "unable to access directory: ${PRESETS_DIR}" >2
    exit 1
fi

JSON_TEMP=$(mktemp preset_builtin.json.XXX)
C_TEMP=$(mktemp preset_builtin.h.XXX)
if [ "${JSON_TEMP:-}" = "" ] || [ "${C_TEMP:-}" = "" ]; then
    echo "unable to create temporary files" >2
    exit 1
fi

trap "rm ${JSON_TEMP} ${C_TEMP}" EXIT INT TERM

"${SELF_DIR}/create_resources.py" preset_builtin.list "${JSON_TEMP}"
echo 'const char hb_builtin_presets_json[] =' > "${C_TEMP}"
"${SELF_DIR}/quotestring.py" "${JSON_TEMP}" >> "${C_TEMP}"
echo ';' >> "${C_TEMP}"
cp "${C_TEMP}" "${LIBHB_DIR}/handbrake/preset_builtin.h"

exit 0
