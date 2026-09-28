# Android port boundaries

## Compatibility invariants

- Original game and mod resources are read-only inputs. The launcher and engine
  must not rewrite `fsgame.ltx`, shaders, textures or archives. The standard
  `<STALKER>/_appdata_` subtree is deliberately writable and owns `user.ltx`,
  saves, screenshots and normal engine logs, as it does on desktop.
- The tracked `res/gamedata` resource tree stays identical to upstream. Android
  compatibility is implemented in engine code and in-memory conversion only.
- Disk formats used by the three PC games remain unchanged. ARM fixes copy
  unaligned packed values instead of changing serialized layouts.
- Hardware selection is capability-based. There are no GPU model, vendor, or
  driver-version allowlists in the Android renderer.
- Platform code is guarded by `XR_PLATFORM_ANDROID`; desktop behavior is not
  silently replaced.

## Components

No tracked upstream file was deleted by the port, and `res/` has no difference
from upstream. The Android platform code is compiled conditionally.

The retained changes fall into these categories:

| Area | Reason retained | Resource impact |
|---|---|---|
| ARMv7 build and dependency fixes | Required to compile the existing engine and LuaJIT with the NDK | None |
| Android filesystem bootstrap | Mounts the selected PC installation and verifies its normal `_appdata_` path | Resources read-only; `_appdata_` writable |
| SDL Activity and diagnostics | Owns the native surface, lifecycle, logs, orientation, and process isolation | None |
| GLES context and framebuffer presentation | Adapts SDL's EGL framebuffer to the existing deferred renderer | None |
| Texture upload compatibility | Decodes unsupported desktop compression in memory when required | Source DDS files unchanged |
| Shader compatibility | Transforms the in-memory compiler input; original and mod shader files are not edited | Source files unchanged |
| Touch input | Emits normal engine keyboard/mouse actions and is optional | No UI asset copies |
