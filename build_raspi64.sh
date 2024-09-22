#!/bin/sh
#
# Example of how to build the SDL2 version of Mini vMac on Linux
# This targets the Raspberry Pi 4 with 64bit OS.
# The resolution is set to 800x600, and Mini vMac is set to launch in fullscreen mode for my purposes.

# we need to build the setup tool first
if [ ! -x ./setup_t ]; then
	gcc -o setup_t setup/tool.c
fi

# run setup tool to generate makefile generator
./setup_t -maintainer "egon.rath@gmail.com" \
        -homepage "https://github.com/egrath" \
        -n "minivmac-3.7-linuxarm64" \
        -e bgc \
        -t larm \
        -cpu a64 \
        -m II \
        -hres 800 -vres 600 -depth 3 \
        -magnify 1 \
        -mf 2 \
        -sound 1 \
        -sony-sum 1 -sony-tag 1 \
        -speed 4 -ta 2 -em-cpu 2 -mem 8M \
        -chr 0 -drc 1 -sss 4 \
        -fullscreen 1 \
        -var-fullscreen 1 \
        -api sd2 \
        > setup.sh

# generate makefile and build
bash -x ./setup.sh
make clean
make -j $(nproc)

