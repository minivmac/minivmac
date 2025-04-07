#!/bin/sh
#
# Example of how to build the SDL2 version of Mini vMac on Linux
#

# we need to build the setup tool first
if [ ! -x ./setup_t ]; then
	gcc -o setup_t setup/tool.c
fi

# run setup tool to generate makefile generator
./setup_t -maintainer "egon.rath@gmail.com" \
        -homepage "https://github.com/egrath" \
        -n "minivmac-3.7-test" \
        -e bgc \
        -t lx64 \
        -m IIx \
        -hres 640 -vres 480 -depth 3 \
        -magnify 1 \
        -mf 2 \
        -sound 1 \
        -sony-sum 1 -sony-tag 1 \
        -speed 4 -ta 2 -em-cpu 2 -mem 32M \
        -chr 0 -drc 1 -sss 4 \
        -fullscreen 0 \
        -var-fullscreen 1 \
        -api sd2 \
        > setup.sh

# generate makefile and build
bash -x ./setup.sh
make clean
make -j $(nproc)

