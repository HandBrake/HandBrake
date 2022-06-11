yum update
yum groupinstall -y "Development Tools" "Additional Development"
yum install -y fribidi-devel git jansson-devel libogg-devel libsamplerate-devel libtheora-devel libvorbis-devel opus-devel speex-devel xz-devel
yum install -y epel-release
yum install -y libass-devel yasm
yum localinstall -y $(curl -L -s 'https://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/o/' | grep -Eo 'opus-[^">]+\.x86_64\.rpm' | sort -u | awk '{ print "https://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/o/"$0 }')
curl -L 'https://nasm.us/nasm.repo' -o /etc/yum.repos.d/nasm.repo
yum install -y nasm
yum localinstall -y --nogpgcheck https://download1.rpmfusion.org/free/el/rpmfusion-free-release-7.noarch.rpm
yum install -y lame-devel x264-devel

git clone -b 1.2.x https://github.com/HandBrake/HandBrake
cd HandBrake
./configure --disable-gtk --disable-x265 --enable-fdk-aac --force
cd build/
make -s -j`nproc`