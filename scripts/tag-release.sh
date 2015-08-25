#! /bin/bash
#
# Usage: tag-release.sh <release-ver> [<commit>]
#
# Creates a new branch and tag for the release
# Optionally, the release can be based off a specific git commit.
#

TAG=${1}
COMMIT=${2}

if [ "x${TAG}" == "x" ]; then
    echo "Missing release tag (e.g. 0.10.0)"
fi

if [ "x${COMMIT}" == "x" ]; then
    echo "Creating release tag ${TAG} and branch ${TAG}-dev from HEAD, proceed?"
else
    echo "Creating release tag ${TAG} and branch ${TAG}-dev from ${COMMIT}, proceed?"
fi
read proceed
if [[ ( "x${proceed}" != "xy" ) && ( "x${proceed}" != "xY" ) ]] ; then
    echo "Aborting..."
    exit 0
fi

if [ "x${COMMIT}" != "x" ]; then
    # create release branch from specific commit
    git checkout "${COMMIT}" -b "${TAG}-dev"
    ERR=$?
else
    # create release branch from head of current branch
    git checkout -b "${TAG}-dev"
    ERR=$?
fi
if [ ${ERR} -ne 0 ]; then
    echo "Failed to create branch ${TAG}-dev"
    exit ${ERR}
fi

# creat tag
git tag -a "${TAG}" -m "Release ${TAG}" HEAD
ERR=$?
if [ ${ERR} -ne 0 ]; then
    echo "Failed to create tag ${TAG}"
    # cleanup... remove the branch that was created
    git branch -d "${TAG}-dev"
    exit ${ERR}
fi

# checkout tag in preparation for building release
# this should put you in a "detached HEAD" state
git checkout "${TAG}"
ERR=$?
if [ ${ERR} -ne 0 ]; then
    echo "Failed to checkout tag ${TAG}"
    # cleanup... remove the branch that was created
    git branch -d "${TAG}-dev"
    exit ${ERR}
fi

remote=$(git config remote.origin.url)
echo
echo "Do you wish to push this release branch and tag to $remote? (y/N)"
echo "You may want to do this manually after creating and verifying release."
echo "e.g."
echo "    git push -u origin ${TAG}-dev"
echo "    git push origin ${TAG}"
read proceed
if [[ ( "x${proceed}" == "xy" ) || ( "x${proceed}" == "xY" ) ]] ; then
    git push -u origin "${TAG}-dev"
    ERR=$?
    if [ ${ERR} -ne 0 ]; then
        echo "Failed to push branch ${TAG}-dev to remote"
        exit ${ERR}
    fi
    git push origin "${TAG}"
    ERR=$?
    if [ ${ERR} -ne 0 ]; then
        echo "Failed to push tag ${TAG}-dev to remote"
        exit ${ERR}
    fi
else
    echo "Branch and tag are local, changes not pushed to remote!"
fi

