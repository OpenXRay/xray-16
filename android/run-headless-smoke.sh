#!/bin/sh
set -eu

if [ "$#" -ne 1 ]; then
    echo "usage: $0 path/to/xr_3da" >&2
    exit 2
fi

engine=$1
if [ ! -x "$engine" ]; then
    echo "engine is not executable: $engine" >&2
    exit 2
fi

exec "$engine" -headless-smoke -no_gamepad -nosplash
