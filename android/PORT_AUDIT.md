# Android port boundaries

This document records what the Android port is allowed to change. It is a
maintenance checklist, not a claim that every game or mod works on Android.

## Filesystem and resource rules

- The selected PC installation is the game-data source.
- `fsgame.ltx`, archives, textures and shader files are read-only inputs.
- Settings, saves, screenshots and logs remain under the installation's normal
  `_appdata_` paths.
- Android shader and texture compatibility changes happen in memory. They must
  not rewrite a user's files.
- Packed on-disk structures keep their PC layout. ARM code must copy unaligned
  fields instead of changing serialized formats.

The fork does change `res/gamedata/configs/text/eng/openxray.xml` to register
the GLES and experimental Vulkan renderer names. The APK also adds generated
`quality_*.ltx` profiles while staging assets. Those are repository-owned
support files; they are not patches applied to the user's installation.

## Platform isolation

Android-specific behavior belongs behind `XR_PLATFORM_ANDROID`, `ANDROID` or
the Android launcher boundary. Changes shared with desktop platforms must be
portable fixes, such as safe unaligned access or standard alignment syntax,
and must continue to pass the desktop CI matrix.

Hardware decisions use reported API features, limits and formats. Do not add
GPU vendor, model or driver-version allowlists.

## Main port areas

| Area | Purpose |
|---|---|
| `android/` | Build scripts, launcher project, packaging and documentation |
| `src/Common` and `src/xrCore` | Android platform definitions, filesystem bootstrap and filename conversion |
| `src/xrEngine` | SDL activity entry point, lifecycle, touch input, diagnostics and renderer selection |
| `src/Layers/xrRenderGL` and `xrRenderPC_GL` | GLES context, framebuffer, shader and texture compatibility |
| `src/Layers/xrRenderVK` | Vulkan probe support, DDS upload and image-state tracking; not a gameplay renderer |
| `src/xrCDB`, `src/xrAICore` and `src/xrGame` | ARM-safe access, collision/visibility fixes and Android runtime stabilization |

## Review checklist

Before accepting an Android change, verify that it:

1. does not modify user-owned game resources;
2. keeps desktop behavior unchanged or intentionally fixes it on every
   platform;
3. selects behavior from capabilities rather than device names;
4. preserves packed file layouts and save compatibility;
5. states whether it was tested by a host unit test, an Android smoke test or
   actual gameplay. These are not interchangeable.
