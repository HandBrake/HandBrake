FROM centos:7
RUN yum install -y centos-release-scl && yum install -y devtoolset-7
RUN yum groupinstall -y "Development Tools" "Additional Development"
RUN yum install -y git sudo fribidi-devel jansson-devel libogg-devel libsamplerate-devel libtheora-devel libvorbis-devel speex-devel xz-devel
RUN yum install -y epel-release
RUN yum install -y libass-devel yasm meson
RUN yum localinstall -y --nogpgcheck https://download1.rpmfusion.org/free/el/rpmfusion-free-release-7.noarch.rpm
RUN yum install -y lame-devel x264-devel
RUN yum localinstall -y `curl -L -s 'https://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/o/' | grep -Eo 'opus-[^">]+\.x86_64\.rpm' | sort -u | awk '{ print "https://archives.fedoraproject.org/pub/archive/epel/6/x86_64/Packages/o/"$0 }'`
RUN yum localinstall -y http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/`curl http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/ |grep -oP 'nasm.*?x86_64.rpm' |tail -1`
RUN yum localinstall -y http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/`curl http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/ |grep -oP 'libvpx.*?x86_64.rpm' |tail -1` \
    yum localinstall -y http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/`curl http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/ |grep -oP 'libvpx-devel.*?x86_64.rpm' |tail -1` \
    yum localinstall -y http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/`curl http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/ |grep -oP 'turbojpeg.*?x86_64.rpm' |tail -1` \
    yum localinstall -y http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/`curl http://mirror.centos.org/centos/8-stream/PowerTools/x86_64/os/Packages/ |grep -oP 'turbojpeg-devel.*?x86_64.rpm' |tail -1`
RUN git clone -b v3.16.3 https://github.com/Kitware/CMake && cd CMake && ./bootstrap && make && sudo make install
ADD build_handbrakeCLI.sh .
WORKDIR /build_hb
CMD /build_handbrakeCLI.sh
