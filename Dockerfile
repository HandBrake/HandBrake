# Build handbrake image
# docker build -t handbrake .

# Run handbrake GUI in a container
# docker run -it --rm \
#        --net=host \
#        --device=/dev/cdrom \
#        -v /tmp/.X11-unix:/tmp/.X11-unix \
#        -v $HOME/.Xauthority:/root/.Xauthority \
#        -v $HOME/handbrake:/handbrake \
#        --env="DISPLAY" \
#        handbrake

# Run HandBrakeCLI in a container
# docker run -it --rm \
#        --entrypoint=HandBrakeCLI \
#        -v $HOME/handbrake:/handbrake \
#        handbrake

# This Dockerfile uses Docker Multi-Stage Builds and requires Docker 17.05+
# See https://docs.docker.com/engine/userguide/eng-image/multistage-build/

# Base Image
FROM ubuntu:16.04 AS base

LABEL maintainer "Micheal Waltz <ecliptik@gmail.com>"

# Environment variables
ENV DEBIAN_FRONTEND=noninteractive \
    LANG=C \
    LANGUAGE=C \
    LC_ALL=C

WORKDIR /handbrake

# Base/Runtime packages
RUN apt update && \
    apt install -y \
          libmp3lame0 \
          libvorbis0a \
          libass5 \
          libsamplerate0 \
          libtheora0 \
          libvorbisenc2 \
          libx264-148  \
          libjansson4 \
          libopus0 \
          libnotify4 \
          libdbus-glib-1-2 \
          libgstreamer-plugins-base1.0-0 \
          libwebkitgtk-3.0-0 \
          libdvd-pkg \
        && \
        apt clean

# Build image
FROM base AS build

# Build packages
RUN apt update && \
    apt install -y \
          autoconf \
          automake \
          build-essential \
	  cmake \
          git \
          libass-dev \
          libbz2-dev \
          libfontconfig1-dev \
          libfreetype6-dev \
          libfribidi-dev \
          libharfbuzz-dev \
          libjansson-dev \
          libmp3lame-dev \
          libogg-dev \
          libopus-dev \
          libsamplerate-dev \
          libtheora-dev \
          libtool \
          libvorbis-dev \
          libx264-dev \
          libxml2-dev \
          m4 \
          make \
          patch \
          pkg-config \
          python \
          tar \
          yasm \
          zlib1g-dev \
          libtool-bin \
          intltool \
          libappindicator-dev \
          libdbus-glib-1-dev \
          libglib2.0-dev \
          libgstreamer1.0-dev \
          libgstreamer-plugins-base1.0-dev \
          libgtk-3-dev \
 	  libgudev-1.0-dev \
          libnotify-dev \
          libwebkitgtk-3.0-dev \
          wget \
          && \
    apt clean

# Copy source to build
COPY . /handbrake/HandBrake
WORKDIR /handbrake/HandBrake
RUN ./configure --launch-jobs=$(nproc) --launch
RUN cd build && make install

# Runtime image
FROM base AS runtime

# Copy HandBrakeCLI and ghb binaries to runtime image
COPY --from=build /handbrake/HandBrake/build/HandBrakeCLI /usr/local/bin
COPY --from=build /handbrake/HandBrake/build/gtk/src/ghb /usr/local/bin

# Make binaries executable
RUN chmod +x /usr/local/bin/HandBrakeCLI
RUN chmod +x /usr/local/bin/ghb

# Runtime command (defaults to HandBrake GUI)
ENTRYPOINT ["ghb"]
