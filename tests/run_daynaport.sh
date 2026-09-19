#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
if ! grep -q '^#define EmDaynaPort 1' cfg/CNFUDALL.h; then
    echo 'Run sh build_daynaport.sh first.' >&2
    exit 1
fi
cc -std=c99 -g -Wall -Wextra -Werror -fsanitize=address,undefined \
    -Icfg -Isrc tests/daynaport.c src/SCSIEMDV.c -o bld/test-daynaport
./bld/test-daynaport
