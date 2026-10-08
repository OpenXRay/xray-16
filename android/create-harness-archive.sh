#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
version=$(sed -n '1p' "$script_dir/PORT_VERSION")
output=${1:-"$repo_dir/build/openxray-android-build-harness-v$version.tar.zst"}

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT HUP INT TERM
archive_name="openxray-android-build-harness-v$version"
archive_root="$stage/$archive_name"
mkdir -p "$archive_root/android" "$archive_root/utils"
cp -a "$repo_dir/android/." "$archive_root/android/"
cp "$repo_dir/utils/validate-glsl-es.py" "$archive_root/utils/validate-glsl-es.py"

{
    echo "OpenXRay Android ARMv7 standalone build harness"
    echo "harness_version=$version"
    echo "source_commit=$(git -C "$repo_dir" rev-parse HEAD)"
    echo "source_branch=$(git -C "$repo_dir" rev-parse --abbrev-ref HEAD)"
    echo "created_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$archive_root/HARNESS-MANIFEST.txt"

mkdir -p "$(dirname -- "$output")"
tar -C "$stage" -I 'zstd -T0 -10' -cf "$output" "$archive_name"
sha256sum "$output"
echo "$output"
