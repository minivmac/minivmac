#!/bin/sh
#
# Example on how to build Mini vMac for DOS using DJGPP
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
	-t mdos \
	-m Plus \
	-km Option Command \
	-api dos \
	> setup.sh

# generate makefile and build
bash -x ./setup.sh
make clean
make -j $(nproc)