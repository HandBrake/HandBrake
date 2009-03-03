#!/bin/sh -ex
#

## This script is invoked by Xcode external targets.
##
## We must guarantee no jobserver is passed through since the file-descriptors
## have been clobbered by Xcode. If this is not done then make behaves as if
## it is allowed to run an infinite number of jobs.
##
MAKEFLAGS=

cd $EXTERNAL_BUILD

## re/configure if not configured by Xcode or if missing top-makefile
if [ $EXTERNAL_METHOD != 'xcode' -o ! -f $EXTERNAL_BUILD/GNUmakefile ]; then
    ## compute --arch value based on Xcode configuration naming convention
    case "$CONFIGURATION" in
    *.i386)
        args="--arch=i386"
        ;;
    *.x86_64)
        args="--arch=x86_64"
        ;;
    *.ppc)
        args="--arch=ppc"
        ;;
    *.ppc64)
        args="--arch=ppc64"
        ;;
    *)
        args=
        ;;
    esac

    ## invoke configure with (hidden) option which indicates conf performed by xcode
    $EXTERNAL_PROJECT/configure PATH=$PATH --conf-method=xcode $args
fi

## safeguard against passing blank value which would result in unlimited jobs
if [ -z "$EXTERNAL_JOBS" ]; then
    jobs=
else
    jobs=--jobs=$EXTERNAL_JOBS
fi

## compute target
case "$1:$2" in
contrib:clean)
    target=contrib.xclean
    ;;
contrib:*)
    target=contrib.install
    ;;
*:clean)
    target=clean
    ;;
libhb:'')
    if [ -z "$EXTERNAL_GOALS" ]; then
        target=build
    else
        target="$EXTERNAL_GOALS"
    fi
    ;;
*:'')
    target=build
    ;;
*)
    echo "usage: $0 PRODUCT ACTION"
    exit 1
    ;;
esac

## handoff
set -x
exec make -C $EXTERNAL_BUILD BUILD.method=xcode $jobs $target
