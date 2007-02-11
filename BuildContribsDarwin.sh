#!/bin/sh
# This script build the contrib libs
# This used on MaxOs X to generate a universal version of the contrib libraries
# The new building process for MediaFork is to download a precompiled universal contrib folder
# and use them to build universal binaries of MediaFork
# pri: 01/28/07
# ########################################################################
# Parameters:
# Package Version  (Version must be incremented to ensure that each of trunk/branches use the correct version)
# IP of the other platform host
# Username to connect with (passwordless must be setup between the 2 machines)
# 
# ie:  ./BuildContribsDarwin.sh 0001 192.168.0.150 toto
#
# #########################################################################
# The script will connect to the other computer, download and build all of the contrib libraries
# and in the same time download and compile the contrib libraries on the native system
# My setup to do that is Mac Pro Intel as native and a Mac mini G4 ppc as foreign
#
# Native is the master computer, it sends compile commands to the foreign, get the library, then do a lipo 
# to assemble both versions and put a universal binary version of the library in contrib/lib folder
#
# Once all of the contrib libraries are builded, a contribbin-darwin-${version}.tar.gz file is created, this file must
# be uploaded to the ftp server so Xcode will be able to build universal binaries of the MediaFork, MediaForkCLI and libmediafork
#
# ##########################################################################
# SSH passwordless setup instructions
# 1) log on native using terminal
# 2) ssh-keygen -t rsa
# 3) leave passphrase empty (hit return)
# 4) copy the $HOME/.ssh/id_rsa.pub to foreign machine $HOME/.ssh/authorized_keys
# ##########################################################################
export VERSION=$1
export IP=$2
export USERNAME=$3
export REPOS=svn://multics.dynalias.com/HandBrake/branches/MediaFork_0.8.0_mpeg4ip
# ##########################################################################
# Launch the build on the foreign system
# ##########################################################################
ssh $USERNAME@$IP "rm -rf MFBUILDTMP ; mkdir MFBUILDTMP ; svn co $REPOS MFBUILDTMP ; cd MFBUILDTMP ; ./configure ; cd contrib ; cp ../config.jam . ; ../jam ; touch BUILDTERMINATED" &
./configure
cd contrib
rm -rf lib include *tar.gz bin share man native foreign
cp ../config.jam .
# Use the new UB jam to be shure to use the correct version to build libraries (native on each system)
../jam
mkdir native
mv lib native
mkdir lib
mkdir foreign
# wait until the foreign build is done :)
wait
echo the foreign build is done, transferring files to native computer:
cd foreign
mkdir lib
cd lib
scp $USERNAME@$IP:/Users/$USERNAME/MFBUILDTMP/contrib/lib/*a .
for lib in `ls *.a`
do	
	echo ... lipo: $lib
	lipo -create $lib ../../native/lib/$lib -output ../../lib/$lib
done;

cd ../..
echo $VERSION > DarwinContribVersion.txt
echo Creating contribbin-darwin-$VERSION.tar.gz
tar zcvf contribbin-darwin-$VERSION.tar.gz lib include  DarwinContribVersion.txt
echo Done....
ls -l contribbin-darwin-$VERSION.tar.gz
cd ..
echo $VERSION > MacOsXContribBinariesVersion.txt
