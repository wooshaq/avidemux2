#!/bin/bash
# Host packages needed by MXE and the Avidemux cross build (Ubuntu 24.04 runner)
set -e
sudo apt-get update -qq
sudo apt-get install -y --no-install-recommends \
  autoconf automake autopoint bash bison bzip2 flex g++ g++-multilib gettext git gperf \
  intltool libc6-dev-i386 libclang-dev libgdk-pixbuf-2.0-dev libltdl-dev libgl-dev libgl1-mesa-dev \
  libpcre2-dev libssl-dev libtool-bin libxml-parser-perl lzip make openssl p7zip-full patch perl \
  python3 python3-mako python3-packaging python3-pkg-resources python3-setuptools python3-yaml \
  python-is-python3 ruby sed sqlite3 unzip wget xz-utils zip nasm yasm pkg-config cmake ninja-build \
  meson file
