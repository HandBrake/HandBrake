#!/bin/bash
# usage: mingw-install-prebuilt [install-dir]

function mingw-install-prebuilt()
{
    local TC_PARAM TC_DIR MINGW_DIR MINGW_TAR MINGW_URL MINGW_MD5 LAS LA

    TC_PARAM="${1}"
    TC_DIR="${HOME}/toolchains"
    if [[ "${TC_PARAM:=}" != "" ]]; then
        TC_DIR="${TC_PARAM}"
    fi

    MINGW_DIR="mingw-w64-x86_64"
    MINGW_TAR="mingw-w64-bin_x86_64-linux_20131228.tar.bz2"
    MINGW_URL="http://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win64/Automated%20Builds/${MINGW_TAR}"
    MINGW_MD5="17669566433998307f894d3476fcdc48"

    if [[ -e "${TC_DIR}/${MINGW_DIR}" ]]; then
        echo "directory exists: ${TC_DIR}/${MINGW_DIR}" >&2
        exit 1
    fi
    mkdir -p "${TC_DIR}/${MINGW_DIR}"
    cd "${TC_DIR}"
    TC_DIR="$(pwd)"

    if [[ ! -e "${MINGW_TAR}" ]] || [[ "$(md5sum ${MINGW_TAR} | awk '{ print $1 }')" != "${MINGW_MD5}" ]]; then
        rm -f "${MINGW_TAR}"
        wget "${MINGW_URL}"
    fi
    cd "${MINGW_DIR}"
    tar -xf "../${MINGW_TAR}"

    LAS=$(find . -type f -name \*.la)
    for LA in ${LAS[@]}; do
        sed -i \
            -e "s;/home/mingw-w64/mingw-w64/linux-x86_64-x86_64/build/build/root/mingw/lib;${TC_DIR}/${MINGW_DIR}/mingw/lib;" \
            -e "s;/home/mingw-w64/mingw-w64/linux-x86_64-x86_64/build/build/root/lib;${TC_DIR}/${MINGW_DIR}/lib;" \
            -e "s;/home/mingw-w64/mingw-w64/linux-x86_64-x86_64/build/build/root/x86_64-w64-mingw32/lib;${TC_DIR}/${MINGW_DIR}/x86_64-w64-mingw32/lib;" \
            "${LA}"
    done

    echo "export PATH=\${PATH}:${TC_DIR}/${MINGW_DIR}/bin" >> ~/.bashrc

    return 0
}

mingw-install-prebuilt "$@"
