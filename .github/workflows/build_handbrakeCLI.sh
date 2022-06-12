#!/usr/bin/scl enable devtoolset-7 -- bash

set -e

export PYTHONHTTPSVERIFY=0

git clone -b 1.2.x https://github.com/HandBrake/HandBrake
cd HandBrake
./configure --disable-gtk --disable-x265 --enable-fdk-aac --force
cd build/
make -s -j`nproc`