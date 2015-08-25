#! /bin/bash
#
# Retrieves git repository info for directory ${1} using command ${2}

# Args
REPO_DIR='.'
if [[ ${1} ]]; then
    REPO_DIR=${1}
fi
GIT_EXE='git'
if [[ ${2} ]]; then
    GIT_EXE=${2}
fi

# Switch to working directory
if ! cd ${REPO_DIR} 2>/dev/null; then
    echo "Invalid directory ${REPO_DIR}." 1>&2
    exit 1
fi

# Check whether we have git
if ! hash ${GIT_EXE} 2>/dev/null; then
    echo "Command '${GIT_EXE}' not found." 1>&2
    exit 1
fi

# Check if there is a valid git repo here
HASH=$(${GIT_EXE} rev-parse HEAD)
ERR=$?
if [[ ${ERR} -ne 0 ]]; then
    echo "Not a valid repository." 1>&2
    exit ${ERR}
elif [[ -z ${HASH} ]]; then
    echo "Not a valid repository." 1>&2
    exit 1
fi

# Retrieve info
URL=$(${GIT_EXE} config remote.origin.url)
TAG=$(${GIT_EXE} describe --abbrev=0)
if [[ ${TAG} ]]; then
    REV=$(${GIT_EXE} rev-list ${TAG}.. --count)
else
    TAG=$(${GIT_EXE} describe $(${GIT_EXE} rev-list --tags --max-count=1))
    if [[ ${TAG} ]]; then
        REV=$(${GIT_EXE} rev-list $(${GIT_EXE} merge-base ${TAG} HEAD).. --count)
    else
        REV=$(${GIT_EXE} rev-list HEAD --count)
    fi
fi
BRANCH=$(${GIT_EXE} symbolic-ref -q --short HEAD)
REMOTE="${URL}"
UPSTREAM=$(${GIT_EXE} config branch.${BRANCH}.remote)
if [[ ${UPSTREAM} ]]; then
    REMOTE="${UPSTREAM}"
fi
DATE=$(${GIT_EXE} log -1 --format="format:%ai")

# Output
# Only write tag and rev if they exist.
echo "URL=${URL}"
echo "HASH=${HASH}"
if [[ ${TAG} ]]; then echo "TAG=${TAG}"; fi
if [[ ${REV} ]]; then echo "REV=${REV}"; fi
echo "BRANCH=${BRANCH}"
echo "REMOTE=${REMOTE}"
echo "DATE=${DATE}"
