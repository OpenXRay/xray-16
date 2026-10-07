# Vulkan renderer: code/host checkpoint #75–85

Closed in this checkpoint: #75–85. This is a source and host verification checkpoint; device image quality remains in #104–145.

| Item | Code path and verification |
| --- | --- |
| 75 | Weather descriptors lease the sky/cloud images, rebind on preset change, and pass interpolated colors, fog and sky rays to the weather shader. The reload transaction preserves the old views when staging fails. |
| 76 | Live light snapshots feed deferred spot/point illumination and shadows. Active glow resources now enqueue textured camera-facing sprites, with descriptor leases released after submitted frames when the level unloads. |
| 77 | Particle effect/group assets, callbacks and fixed-step simulation feed transparent/HUD buffers. Triggered children and pending events are also discarded on device teardown. |
| 78 | Rain, flare and thunderbolt paths feed transparent/UI draws from gameplay weather callbacks. Destructors remove queued rain/bolt pointers before freeing frame buffers. |
| 79 | Alpha test writes G-buffer, transparent level/model draws are sorted after opaque lighting, and glass/wallmarks retain their dedicated material paths. The game transparent fragment now reconstructs world position and applies sun/ambient plus up to 16 nearby spot/point sources. Static and GPU-skinned paths bind their per-image uniform at set 1 and set 2 respectively. HUD and effect sprites use the explicit unlit variant. The SPIR-V descriptor sets, offsets and host pipeline fixture were verified. |
| 80 | World, transparency, HUD and UI precede postprocess; final readback feeds JPEG, TGA and game-save DDS encoders. Existing screen-copy and frame-phase fixtures cover the host pass order. |
| 81–82 | Level unload/replacement waits for submitted work, drops scene queues and wallmarks, resets the pool/revision and releases level buffers. The three-profile asset-flow fixture exercises sequential load, failed replacement, draw and teardown. |
| 83 | `OnAssetsChanged` defers reload to the next frame boundary. It stages all loaded DDS replacements, waits for uploads, retires old weather descriptors immediately before commit, rebinds their new views, and releases old images only after descriptor updates. A corrupt DDS leaves previous resources and weather leases intact in the asset-flow mock fixture. |
| 84–85 | OGF/OMF motion slots, references, bone remap, clip playback and callbacks reach `VulkanKinematics`; independent visual copies own their pose/blends while sharing GPU model data. Kinematics and asset-flow fixtures exercise two different playback times, separate pose buffers and deletion of one copy. |


Debug/Release `xrRenderVK`, the full host CTest suite, shader validation and the new reload failure case are the code gates. No new Android APK or device frame was produced here.
