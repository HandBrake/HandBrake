#!/usr/bin/scl enable devtoolset-7 -- bash

set -e

export PYTHONHTTPSVERIFY=0

git clone --single-branch --depth=1 https://github.com/HandBrake/HandBrake
cd HandBrake
./configure --launch-jobs=$(nproc) --launch --disable-gtk --enable-fdk-aac
cd build/ && find
