#!/bin/sh
#
# Example of how to build the SDL2 version of Mini vMac on Linux for Windows (and also
# on MSYS2)
#
# Building on Windows is currently a little tricky as the SDL2 target isn't fully supported
# there.
#
# Instructions for RHEL:
#
#   1. Install the Mingw64 toolchain:
#       sudo dnf install mingw64-gcc-c++.x86_64 
#
#   2. Download and install SDL2 for Mingw64:
#       wget https://www.libsdl.org/release/SDL2-devel-2.0.22-mingw.tar.gz
#       gzip -dc SDL2-devel-2.0.22-mingw.tar.gz | tar -xvf -
#       cd SDL2-2.0.22
#       sudo make install-package arch=x86_64-w64-mingw32 prefix=/usr/x86_64-w64-mingw32
#
#   3. Create link for some files in the source directory: (only needed in Linux)
#       ln -s /usr/bin/x86_64-w64-mingw32-gcc gcc.exe
#       ln -s /usr/bin/x86_64-w64-mingw32-windres windres.exe
#       ln -s /usr/bin/x86_64-w64-mingw32-strip strip.exe
#       ln -s src SRC
#

# we need to build the setup tool first
if [ ! -x ./setup_t ]; then
	gcc -o setup_t setup/tool.c
fi

# run setup tool to generate makefile generator
./setup_t -maintainer "egon.rath@gmail.com" \
        -homepage "https://github.com/egrath" \
        -n "minivmac-3.7-test" \
        -e mgw \
        -t wx64 \
        -m II \
        -hres 640 -vres 480 -depth 3 \
        -magnify 1 \
        -mf 2 \
        -sound 1 \
        -sony-sum 1 -sony-tag 1 \
        -speed 4 -ta 2 -em-cpu 2 -mem 8M \
        -chr 0 -drc 1 -sss 4 \
        -fullscreen 0 \
        -var-fullscreen 1 \
        -api sd2 \
        > setup.sh

# patch the setup file for Windows SDL2 issue with WinMain undefined reference
sed -i '/.*SDL\.h/i printf \"%s\\n\" \"#define SDL_MAIN_HANDLED\" \>\> \"\$\{DestFile\}\"' setup.sh

# generate makefile
bash -x ./setup.sh

# build
export PATH=.:$PATH
make clean
make -j $(nproc)

