#!/bin/sh
set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
repo_dir=${1:-$(CDPATH= cd -- "$script_dir/.." && pwd)}
patch_dir=${XRAY_ANDROID_PATCH_DIR:-"$script_dir/patches"}

patchset_present()
{
    expected_version=$(sed -n '1p' "$script_dir/PORT_VERSION" 2>/dev/null || true)
    target_version=$(sed -n '1p' "$repo_dir/android/PORT_VERSION" 2>/dev/null || true)
    [ -n "$expected_version" ] \
        && [ "$target_version" = "$expected_version" ] \
        && [ -x "$repo_dir/android/prepare-source.sh" ] \
        && [ -s "$repo_dir/src/Layers/xrRenderPC_GL/AndroidGlslCompatRules.inl" ] \
        && grep -Fq 'AndroidGlslCompatRules.inl' \
            "$repo_dir/src/Layers/xrRenderPC_GL/rgl_shaders.cpp" \
        && grep -Fq '#define skin_input_normal(value)' \
            "$repo_dir/src/Layers/xrRenderPC_GL/rgl_shaders.cpp" \
        && grep -Fq 'glBindFramebuffer(GL_READ_FRAMEBUFFER, pFB)' \
            "$repo_dir/src/Layers/xrRenderGL/glHW.cpp" \
        && grep -Fq 'GL_TEXTURE_SWIZZLE_R' \
            "$repo_dir/src/Layers/xrRenderGL/glTexture.cpp" \
        && grep -Fq 'pw->pw_gecos && pw->pw_gecos[0]' \
            "$repo_dir/src/xrCore/xrCore.cpp" \
        && grep -Fq 'android_native_crash_handler' \
            "$repo_dir/src/xrEngine/x_ray.cpp" \
        && grep -Fq 'pc-libmain' "$repo_dir/src/xrEngine/x_ray.cpp" \
        && grep -Fq 'AndroidTouchControlMask' \
            "$repo_dir/src/xrEngine/android_touch_controls.cpp" \
        && grep -Fq 'SCREEN_ORIENTATION_LANDSCAPE' \
            "$repo_dir/android/apk/app/src/main/java/org/openxray/app/XRayActivity.java" \
        && grep -Fq 'EXTRA_TOUCH_CONTROLS' \
            "$repo_dir/android/apk/app/src/main/java/org/openxray/app/LauncherActivity.java" \
        && grep -Fq 'CopyMemory(&tableSize, current_cross_table' \
            "$repo_dir/src/xrAICore/Navigation/game_graph_inline.h" \
        && grep -Fq 'LUAJIT_HOST_EXTRA_LDFLAGS' \
            "$repo_dir/Externals/LuaJIT-proj/CMakeLists.txt" \
        && grep -Fq 'm_Overlay' "$repo_dir/src/xrCore/LocatorAPI_defs.h" \
        && grep -Fq 'profilePreference(PREF_GAME_PATH_PREFIX' \
            "$repo_dir/android/apk/app/src/main/java/org/openxray/app/LauncherActivity.java" \
        && grep -Fq "versionName portVersion[0]" "$repo_dir/android/apk/app/build.gradle"
}

if patchset_present; then
    echo "Android patchset: already present"
    exit 0
fi

if [ ! -d "$patch_dir" ]; then
    echo "Android patchset is missing: $patch_dir" >&2
    exit 2
fi

if ! git -C "$repo_dir" diff --quiet || ! git -C "$repo_dir" diff --cached --quiet; then
    echo "Android patchset is incomplete, but the repository has local changes; commit or stash them before applying patches" >&2
    exit 2
fi

found_patch=false
for patch in "$patch_dir"/*.patch; do
    [ -f "$patch" ] || continue
    found_patch=true
    if git -C "$repo_dir" apply --check --3way "$patch" >/dev/null 2>&1; then
        git -C "$repo_dir" apply --3way "$patch"
        echo "Android patchset: applied $(basename "$patch")"
    elif git -C "$repo_dir" apply --reverse --check "$patch" >/dev/null 2>&1; then
        echo "Android patchset: $(basename "$patch") is already applied"
    else
        echo "Android patchset cannot be applied cleanly: $patch" >&2
        exit 2
    fi
done

if [ "$found_patch" = false ]; then
    echo "Android patchset directory is empty: $patch_dir" >&2
    exit 2
fi

if ! patchset_present; then
    echo "Android patchset was applied but failed its integrity check" >&2
    exit 2
fi
