#!/bin/sh
# Run after generating a configuration with disk format detection enabled.
set -eu
cd "$(dirname "$0")/.."
case $(uname -s) in
    Darwin)
        link_gc=-Wl,-dead_strip
        pbuf_lang='-x objective-c -framework Cocoa'
        ;;
    Linux)
        link_gc=-Wl,--gc-sections
        pbuf_lang=
        ;;
    *) echo 'This test runner currently supports macOS and Linux.' >&2; exit 1 ;;
esac
mkdir -p bld
${CC:-cc} -std=c99 -g -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter \
    -ffunction-sections -fdata-sections -fsanitize=address,undefined \
    -Icfg -Isrc tests/disk_images.c "$link_gc" -o bld/test-disk-images
UBSAN_OPTIONS=halt_on_error=1 ./bld/test-disk-images "$@"

${CC:-cc} -std=c99 -g -Wall -Wextra -Werror \
    -Wno-unused-function -Wno-unused-parameter \
    $pbuf_lang -Icfg -Isrc tests/pbuf_zero.c -o bld/test-pbuf-zero
./bld/test-pbuf-zero
