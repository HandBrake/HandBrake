#! /bin/bash
#
# Retrieves git repository info for directory ${1} using command ${2}

function repo_info()
{
    local repo_dir git_exe commit upstream err

    # Process args
    repo_dir='.'
    if [[ ${1} ]]; then
        repo_dir=${1}
    fi
    git_exe='git'
    if [[ ${2} ]]; then
        git_exe=${2}
    fi

    # Switch to working directory
    if ! cd ${repo_dir} 2>/dev/null; then
        echo "Invalid directory ${repo_dir}." 1>&2
        return 1
    fi

    # Check whether we have git
    if ! hash ${git_exe} 2>/dev/null; then
        echo "Command '${git_exe}' not found." 1>&2
        return 1
    fi

    # Check if there is a valid git repo here
    HASH=$(${git_exe} rev-parse HEAD)
    SHORTHASH=$(${git_exe} rev-parse --short HEAD)
    err=$?
    if [[ ${err} -ne 0 ]]; then
        echo "Not a valid repository." 1>&2
        return ${err}
    elif [[ -z ${HASH} ]]; then
        echo "Not a valid repository." 1>&2
        return 1
    fi

    # Retrieve info
    URL=$(${git_exe} config remote.origin.url)

    # check if an annotated tag is reachable from HEAD
    TAG=$(${git_exe} describe --tags --abbrev=0 --exact-match --match \[0-9\]\*.\[0-9\]\*.\[0-9\]\* HEAD 2> /dev/null)
    if [[ ${TAG} ]]; then
        # if TAG is a release tag and HASH == TAG_HASH, this is release code
        TAG_HASH=$(${git_exe} rev-list ${TAG} --max-count=1)
        REV=$(${git_exe} rev-list $(${git_exe} merge-base ${TAG} HEAD).. --count)
    else
        REV=$(${git_exe} rev-list HEAD --count)
    fi

    BRANCH=$(${git_exe} symbolic-ref -q --short HEAD)
    REMOTE="${URL}"
    upstream=$(${git_exe} config branch.${BRANCH}.remote)
    if [[ ${upstream} ]]; then
        REMOTE="${upstream}"
    fi
    DATE=$(${git_exe} log -1 --format="format:%ci")

    # Output
    # Only write tag and rev if they exist.
    echo "URL=${URL}"
    echo "HASH=${HASH}"
    echo "SHORTHASH=${SHORTHASH}"
    if [[ ${TAG} ]]; then echo "TAG=${TAG}"; fi
    if [[ ${TAG_HASH} ]]; then echo "TAG_HASH=${TAG_HASH}"; fi
    if [[ ${REV} ]]; then echo "REV=${REV}"; fi
    echo "BRANCH=${BRANCH}"
    echo "REMOTE=${REMOTE}"
    echo "DATE=${DATE}"

    return 0
}

repo_info "$@"
