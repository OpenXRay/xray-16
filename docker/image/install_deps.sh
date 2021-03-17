#!/bin/sh

# -e == exits as soon as any line in the script fails.
# -x == prints each command that is going to be executed with a little plus.
set -e -x

# update system
apt-get update

export DEBIAN_FRONTEND=noninteractive

# install compiler and tools
apt-get install -y \
  ssh \
  make \
  cmake \
  ccache \
  git \
  gcc-10 \
  g++-10 \
  clang-10 \
  clang-format-10 \
  clang-tidy-10 \
  cppcheck \
  ca-certificates \
  openssh-server \
  rsync \
  lldb-10 \
  vim \
  gdb \
  wget \
  autoconf \
  libglew-dev \
  libfreeimage-dev \
  libfreeimageplus-dev \
  liblockfile-dev \
  libopenal-dev \
  libcrypto++-dev \
  libogg-dev \
  libtheora-dev \
  libvorbis-dev \
  libsdl2-dev \
  liblzo2-dev \
  libjpeg-dev \
  libreadline-dev
