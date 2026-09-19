#!/bin/sh
# Start the macOS DaynaPORT build with its shared vmnet backend enabled.
set -eu
cd "$(dirname "$0")"
exec sudo -n env MINIVMAC_DAYNAPORT=shared \
    ./minivmac.app/Contents/MacOS/minivmac "$@"
