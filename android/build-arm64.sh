#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
XRAY_ANDROID_ABI=arm64-v8a exec "$script_dir/build-android-native.sh" "$@"
