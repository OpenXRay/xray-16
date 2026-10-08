#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=${1:-$(CDPATH= cd -- "$script_dir/.." && pwd)}

source_tree_complete()
{
    [ -f "$repo_dir/Externals/LuaJIT/src/lj_arch.h" ] \
        && [ -f "$repo_dir/Externals/imgui/imgui.cpp" ] \
        && [ -f "$repo_dir/Externals/luabind/luabind/luabind.hpp" ] \
        && [ -f "$repo_dir/Externals/gli/gli/gli.hpp" ] \
        && [ -f "$repo_dir/Externals/xrLuaFix/lfs/src/lfs.c" ]
}

is_git_checkout=false
if git -C "$repo_dir" rev-parse --is-inside-work-tree >/dev/null 2>&1 \
    && [ -f "$repo_dir/.gitmodules" ]; then
    is_git_checkout=true
fi

submodules_match_checkout()
{
    [ "$is_git_checkout" = false ] \
        || ! git -C "$repo_dir" submodule status --recursive | grep -Eq '^[+-U]'
}

if source_tree_complete && submodules_match_checkout; then
    echo "Android source dependencies: ready"
    exit 0
fi

if [ "$is_git_checkout" = true ]; then
    echo "Android source dependencies: initializing recursive git submodules"
    git -C "$repo_dir" submodule sync --recursive
    git -C "$repo_dir" submodule update --init --recursive --depth 1
fi

if ! source_tree_complete; then
    echo "OpenXRay source dependencies are incomplete." >&2
    echo "Use a recursive checkout (git clone --recurse-submodules) or the complete source archive." >&2
    exit 2
fi

echo "Android source dependencies: ready"
