#!/bin/sh
#
# Usage: tag-release.sh <release-ver> [<ref>]
#
# Creates a new bugfix branch and release tag for a release.
# Optionally, the release can be based off a specific git ref.
# Default is HEAD of the current branch.
#
# The script does a lot of error checking, but boils down to
# the following actions.
#
# For major/minor releases (X.Y.0) from master branch:
#   git checkout -b X.Y.x
#   git tag -a X.Y.Z -m "X.Y.Z" HEAD
# and optionally:
#   git push -u origin X.Y.x
#   git push origin refs/tags/X.Y.Z
#
# For bugfix releases from associated branch (X.Y.x):
#   git tag -a X.Y.Z -m "X.Y.Z" HEAD
# and optionally:
#   git push origin refs/tags/X.Y.Z
#

GIT_EXE='git'

validate_repo()
{
    local HASH err AHEAD BEHIND proceed

    # Check whether we have git
    if [ -z "$(command -v ${GIT_EXE})" ]; then
        echo "Command '${GIT_EXE}' not found." 1>&2
        return 1
    fi

    # Check if there is a valid git repo here
    HASH=$(${GIT_EXE} rev-parse HEAD)
    err=$?
    if [ ${err} -ne 0 ]; then
        echo "Not a valid repository." 1>&2
        return ${err}
    elif [ -z "${HASH}" ]; then
        echo "Not a valid repository." 1>&2
        return 1
    fi

    if [ -n "$(${GIT_EXE} status --porcelain)" ]; then
        echo "There are uncommitted changes.  Aborting." 1>&2
        return 1
    fi

    echo "Fetching repo data..."
    ${GIT_EXE} fetch
    err=$?
    if [ ${err} -ne 0 ]; then
        echo "Failed to fetch repo data." 1>&2
        return ${err}
    fi
    AHEAD=$(${GIT_EXE} rev-list @{u}..HEAD --count)
    BEHIND=$(${GIT_EXE} rev-list HEAD..@{u} --count)
    if [ ${AHEAD} -ne 0 ]; then
        echo "There are unpushed changes. Continue anyway? (y/N)"
        read proceed
        if [ "x${proceed}" != "xy" ] && [ "x${proceed}" != "xY" ] ; then
            echo "Aborting..."
            return 1
        fi
    fi
    if [ ${BEHIND} -ne 0 ]; then
        echo "There are unmerged upstream changes. Continue anyway? (y/N)"
        read proceed
        if [ "x${proceed}" != "xy" ] && [ "x${proceed}" != "xY" ] ; then
            echo "Aborting..."
            return 1
        fi
    fi
}

tag_release()
{
    local TAG REF COMMIT BRANCH proceed new_branch ERR HASH

    TAG=${1}
    REF=${2}

    if [ "x${TAG}" = "x" ]; then
        echo "Missing release tag (e.g. 1.0.0)"
        echo "Usage: tag-release.sh tag [commit]"
        return 1
    fi

    # bugfix branch name
    BRANCH=${TAG%.[0-9]*}.x

    if [ "x${REF}" = "x" ]; then
        echo "Creating release tag ${TAG} and branch ${BRANCH} from HEAD, proceed? (y/N)"
        # retrieve full hash of HEAD
        COMMIT=$(${GIT_EXE} rev-list HEAD --max-count=1)
    else
        echo "Creating release tag ${TAG} and branch ${BRANCH} from ${REF}, proceed? (y/N)"
        # retrieve full hash from ref or short hash
        COMMIT=$(${GIT_EXE} rev-list ${REF} --max-count=1)
    fi
    read proceed
    if [ "x${proceed}" != "xy" ] && [ "x${proceed}" != "xY" ] ; then
        echo "Aborting..."
        return 0
    fi

    # check if the remote branch already exists
    ${GIT_EXE} rev-parse --quiet --verify origin/${BRANCH} > /dev/null
    if [ $? -ne 0 ]; then
        # remote branch does not exist
        new_branch=1
        # does the branch already exist locally?
        ${GIT_EXE} rev-parse --quiet --verify ${BRANCH} > /dev/null
        if [ $? -ne 0 ]; then
            # local branch does not exist
            # create bugfix branch from commit
            ${GIT_EXE} checkout "${COMMIT}" -b "${BRANCH}"
            ERR=$?
            if [ ${ERR} -ne 0 ]; then
                echo "Failed to create branch ${BRANCH}"
                return ${ERR}
            fi
        else
            # local branch already exists
            # When the branch already exists, make sure it is being used!
            current_branch=$(${GIT_EXE} rev-parse --abbrev-ref HEAD)
            if [ "$current_branch" != "${BRANCH}" ]; then
                echo "You did not checkout the correct branch ${BRANCH} for tag ${TAG}"
                return 1
            fi
        fi
    else
        new_branch=0
        # When the branch already exists, make sure it is being used!
        current_branch=$(${GIT_EXE} rev-parse --abbrev-ref HEAD)
        if [ "$current_branch" != "${BRANCH}" ]; then
            echo "You did not checkout the correct branch ${BRANCH} for tag ${TAG}"
            return 1
        fi
    fi

    # at this point we should be at the head of the tracking branch
    # for this release.  Make certain that HEAD matches COMMIT
    HASH=$(${GIT_EXE} rev-list HEAD --max-count=1)
    if [ ${HASH} != ${COMMIT} ]; then
        echo "Commit specified does not match current branch HEAD"
        return 1
    fi

    # create tag
    ${GIT_EXE} tag -a "${TAG}" -m "${TAG}" HEAD
    ERR=$?
    if [ ${ERR} -ne 0 ]; then
        echo "Failed to create tag ${TAG}"
        # cleanup... remove the branch that was created
        ${GIT_EXE} branch -d "${BRANCH}"
        return ${ERR}
    fi

    # checkout tag in preparation for building release
    # this should put you in a "detached HEAD" state
    ${GIT_EXE} checkout "${TAG}"
    ERR=$?
    if [ ${ERR} -ne 0 ]; then
        echo "Failed to checkout tag ${TAG}"
        # cleanup... remove the branch that was created
        ${GIT_EXE} branch -d "${BRANCH}"
        return ${ERR}
    fi

    remote=$(${GIT_EXE} config remote.origin.url)
    echo
    echo "Do you wish to push this release branch and tag to $remote? (y/N)"
    echo "You may want to do this manually after creating and verifying release."
    echo "e.g."
    echo "    git push -u origin ${BRANCH}"
    echo "    git push origin refs/tags/${TAG}"
    read proceed
    if [ "x${proceed}" = "xy" ] || [ "x${proceed}" = "xY" ] ; then
        if [ $new_branch .eq 1 ]; then
            ${GIT_EXE} push -u origin "${BRANCH}"
            ERR=$?
            if [ ${ERR} -ne 0 ]; then
                echo "Failed to push branch ${BRANCH} to remote"
                return ${ERR}
            fi
        fi
        ${GIT_EXE} push origin refs/tags/"${TAG}"
        ERR=$?
        if [ ${ERR} -ne 0 ]; then
            echo "Failed to push tag ${BRANCH} to remote"
            return ${ERR}
        fi
    else
        echo "Branch and tag are local, changes not pushed to remote!"
    fi

}

main()
{
    if validate_repo; then
        tag_release "$@"
    else
        return $?
    fi
}

main "$@"
