#!/bin/sh
set -eu
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
XRAY_ANDROID_ABI=armeabi-v7a exec "$script_dir/build-android-native.sh" "$@"
