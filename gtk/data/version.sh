#!/bin/sh

GIT_TAG="`git describe --tags --exact-match HEAD 2>/dev/null || true`"
GIT_REV="`git rev-parse --short HEAD 2>/dev/null`"
if [ -n "$GIT_TAG" ]; then
    echo "$GIT_TAG"
else
    GIT_TAG="`git describe --tags --abbrev=0 HEAD 2>/dev/null`"
    echo "${GIT_TAG:-unknown}${GIT_REV:+-git-$GIT_REV}"
fi
exit 0
