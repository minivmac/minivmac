#!/bin/sh
# Experimental Apple Silicon / Macintosh Plus networking build.
set -eu
cd "$(dirname "$0")"
cc -o setup_t setup/tool.c
./setup_t -n minivmac -t mcar -m Plus -api cco -e bgc \
    -mem 4M -daynaport -sgn 0 > setup.sh
sh setup.sh
# The setup tool changes generated configuration headers. Force all objects to
# rebuild so a preceding non-networking build cannot leave incompatible code.
make -B -j4
