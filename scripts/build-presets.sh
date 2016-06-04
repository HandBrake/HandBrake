#!/bin/bash
# usage: build-presets

SELF="${BASH_SOURCE[0]}"
BASE_DIR=$(cd $(dirname "${BASH_SOURCE[0]}") && pwd -P)
BASE_DIR="${BASE_DIR:-$(pwd)}"
LIBHB_DIR="${BASE_DIR}/../libhb"
PRESETS_DIR="${BASE_DIR}/../preset"

if ! cd "${PRESETS_DIR}"; then
    exit 1
fi

JSON_TEMP=$(mktemp hb_builtin_presets.json.XXX)
C_TEMP=$(mktemp hb_builtin_presets.h.XXX)
if [[ "${JSON_TEMP:-}" == "" ]] || [[ "${C_TEMP:-}" == "" ]]; then
    echo "unable to create temporary file" >2
    exit 1
fi
"${BASE_DIR}/create_resources.py" preset_builtin.list "${JSON_TEMP}" \
&& echo 'const char hb_builtin_presets_json[] =' > "${C_TEMP}" \
&& "${BASE_DIR}/quotestring.py" "${JSON_TEMP}" >> "${C_TEMP}" \
&& echo ';' >> "${C_TEMP}" \
&& cp "${C_TEMP}" "${LIBHB_DIR}/preset_builtin.h"
rm "${JSON_TEMP}" "${C_TEMP}"

exit 0
