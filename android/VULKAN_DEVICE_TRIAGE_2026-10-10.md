# Vulkan device follow-up after 0.9.131

Evidence: `android_20261009_151821_627_21877_3bd95c64.log`, five screenshots of Skadovsk, the campfire and vegetation. No crash was recorded in this run. The source game and mod resources are treated as inputs and are not modified.

## Confirmed engine-side discrepancies

- Local lights were present: at frame 3000, 32 visible lights (13 spots) with summed luminance 19.956; at frame 4800, 25 visible (14 spots), luminance 15.089. A missing individual fixture is not the general cause of the dark interior.
- Vulkan attenuated every local light as `(1 - d / r)^2`; the original `lmodel.h::plight_local` uses `saturate(1 - d² / r²)`. At half range these are 0.25 and 0.75 respectively. The deferred shader, forward shader and CPU object-light estimate now follow the original formula. Device appearance is not yet verified.
- `VulkanParticleGroup::OnFrame` started and stopped a short effect in the same update. The original group selects **one** transition according to its state at the beginning of the frame. The Vulkan group now follows this order; the campfire flame remains subject to a device check.
- A previous Vulkan-only menu special case hardcoded a gold selection tint. Removed its assignment, fragment branch and blend mode. The renderer again follows the parsed game's `shaders.xr` blend mode and untouched UI textures.

## Open renderer-wide parity gaps

The original deferred combine pass (`res/gamedata/shaders/gl/combine_1.ps`, `hmodel.h`, `lmodel.h`) carries diffuse, gloss and material information, samples two environment cubemaps and a material lookup, adds RGB ambient and local diffuse/specular light, then applies fog and tone mapping. Vulkan currently stores albedo and normal with the static hemi/sun masks, reduces ambient and hemi to luminance scalars, uses diffuse-only local lights and grades the base before additive local lighting. These are renderer architecture gaps that explain why changing individual light intensities cannot establish visual parity. Preserve game data and implement these channels in the renderer with a controlled comparison of the same save and weather.

The fixed 1024 sun shadow atlas over 180 m also remains a candidate for the coarse square shadows. The Extreme run is already around 13–14 FPS, frame p95 ~70–91 ms, so increasing shadow resolution or vegetation radius without reducing other work would worsen frame time. Stalls and world draw distance remain open engine/render issues.
