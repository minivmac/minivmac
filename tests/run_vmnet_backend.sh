#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
mkdir -p bld
cc -g -Wall -Wextra -Werror -fblocks -fsanitize=address,undefined \
    -Isrc tests/vmnet_backend.m -framework Foundation -framework vmnet \
    -o bld/test-vmnet-backend
./bld/test-vmnet-backend
