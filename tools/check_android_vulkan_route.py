#!/usr/bin/env python3
"""Audit the packaged Android Vulkan route; runtime selection is tested by CTest."""
import argparse
from pathlib import Path
import re
import subprocess
from zipfile import ZipFile


def check(repo, sdl, native=None, readelf=None, apk=None):
    errors = []
    def require(path, pattern, reason):
        if not re.search(pattern, path.read_text(), re.S):
            errors.append(reason)
    java = repo / 'android/apk/app/src/main/java/org/openxray/app'
    require(java / 'OptionCatalog.java', r'RENDERER_ARGS.*?-renderer-auto.*?-renderer-gles.*?-renderer-vulkan', 'launcher renderer option table changed')
    require(java / 'XRayActivity.java', r'getLibraries\(\).*?"main"', 'SDL gameplay library is not main')
    require(java / 'XRayActivity.java', r'getArguments\(\).*?OptionCatalog\.RENDERER_ARGS.*?args.add\(rendererArg\)', 'gameplay renderer argument is not forwarded')
    require(sdl / 'android-project/app/src/main/java/org/libsdl/app/SDLActivity.java', r'getArguments\(\).*?nativeRunMain\(library, function, arguments\)', 'SDL JNI argument forwarding changed')
    require(sdl / 'src/video/android/SDL_androidwindow.c', r'if \(window->flags & SDL_WINDOW_OPENGL\)\s*\{\s*data->egl_surface = SDL_EGL_CreateSurface', 'SDL EGL surface is not guarded by the OpenGL window flag')
    require(repo / 'src/xr_3da/entry_point.cpp', r'vulkan::GetRendererModule\(\)', 'Vulkan module is not registered in native gameplay')
    require(repo / 'src/xrEngine/x_ray.cpp', r'if \(xray::render::android_native_splash_allowed\(commandLine\)\)', 'Android software splash can precede Vulkan selection')
    require(repo / 'src/xrEngine/Engine.cpp', r'choice !=.*?VulkanUnavailable.*?Explicit Vulkan renderer unavailable', 'explicit Vulkan probe failure has no diagnostic')
    require(repo / 'src/xrEngine/EngineAPI.cpp', r'!selectedRenderer &&.*?renderer_vulkan.*?R_ASSERT2\(false', 'explicit Vulkan can fall back after module selection')
    if native:
        symbols = subprocess.check_output([str(readelf), '--symbols', '--wide', str(native)], text=True)
        if 'VulkanLevelRender' not in symbols or 'SDL_main' not in symbols:
            errors.append('native gameplay lacks Vulkan renderer or SDL JNI entry point')
    if apk:
        with ZipFile(apk) as archive:
            names = set(archive.namelist())
            for name in ('libmain.so', 'libopenal.so', 'libc++_shared.so'):
                if 'lib/armeabi-v7a/' + name not in names:
                    errors.append('APK dependency missing: ' + name)
            if 'classes.dex' not in names:
                errors.append('APK Java launcher classes missing')
    return errors


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument('--sdl-root', type=Path, required=True)
    parser.add_argument('--native-lib', type=Path)
    parser.add_argument('--readelf', type=Path)
    parser.add_argument('--apk', type=Path)
    args = parser.parse_args()
    if args.native_lib and not args.readelf:
        parser.error('--native-lib requires --readelf')
    errors = check(args.repo, args.sdl_root, args.native_lib, args.readelf, args.apk)
    for error in errors:
        print(error)
    if not errors:
        print('Android Vulkan route and package checks passed')
    raise SystemExit(bool(errors))
