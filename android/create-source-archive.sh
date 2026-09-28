#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=$(CDPATH= cd -- "$script_dir/.." && pwd)
version=$(sed -n '1p' "$script_dir/PORT_VERSION")
output=${1:-"$repo_dir/build/openxray-android-source-v$version.tar.zst"}

"$script_dir/prepare-source.sh" "$repo_dir"
if [ -n "$(git -C "$repo_dir" status --porcelain --untracked-files=no --ignore-submodules=none)" ]; then
    echo "Refusing to archive a source tree with tracked or submodule changes" >&2
    exit 2
fi

stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT HUP INT TERM
archive_name="openxray-android-source-v$version"
archive_root="$stage/$archive_name"
mkdir -p "$archive_root"

# Copy only files tracked by the main repository and its recursive submodules.
# Build outputs, local caches, credentials and .git metadata are never included.
git -C "$repo_dir" ls-files --recurse-submodules -z \
    | rsync -a --from0 --files-from=- "$repo_dir/" "$archive_root/"

{
    echo "OpenXRay Android ARMv7 complete source"
    echo "source_version=$version"
    echo "source_commit=$(git -C "$repo_dir" rev-parse HEAD)"
    echo "source_branch=$(git -C "$repo_dir" rev-parse --abbrev-ref HEAD)"
    echo "source_date=$(git -C "$repo_dir" show -s --format=%cI HEAD)"
    echo "submodules=recursive tracked contents included"
} > "$archive_root/SOURCE-MANIFEST.txt"

mkdir -p "$(dirname -- "$output")"
tar -C "$stage" -I 'zstd -T0 -10' -cf "$output" "$archive_name"
sha256sum "$output"
echo "$output"
