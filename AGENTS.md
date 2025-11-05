# AGENTS.md – Assistant Quick Guide

## Mission Snapshot
- Modernise OpenXRay's animation runtime by integrating ozz-animation while keeping legacy behaviour intact.
- xrAnimation module owns the façade around `OzzKinematics`, converter tools, parity tests, and the in-engine `.ozzx` visual.
- Converter CLI already produces `.ozz/.ozzx` assets from legacy `.ogf/.omf`; viewer tooling validates bind pose and animation parity.
- Minimum Viable Product is shipped: the runtime façade, converter, and bundle visual now mirror legacy behaviour for the shipped fixtures.

## Active Priorities
1. Harden the MVP: keep regression suites green, extend automation around `.ozzx` bundle loading, and react quickly to edge cases raised in gameplay smoke tests.
2. Capture and publish lightweight telemetry comparing legacy vs. Ozz frame costs; feed the numbers back into docs and tuning guidelines.
3. Shape the next phase (threading, GPU skinning, richer metadata) by collecting follow-up requirements and documenting the proposed roadmap.

## Workflow Expectations
- Read existing memory (docs, notes) before running commands or changing files.
- Prefer Debug/Mixed builds for iteration; use engine containers (`xr_vector`, `shared_str`, etc.) and `Msg()` for logging.
- After every code or doc change: rebuild the affected targets and rerun their tests without waiting for a prompt.
- Keep responses concise, factual, and professional; update documentation immediately when behaviour changes.

## Build & Test Quickstart
- Configure (if needed): `cmake -S xray-16 -B ozz_utils -DCMAKE_BUILD_TYPE=Debug`
- Build animation targets: `cmake --build ozz_utils --target ozz_kinematics_tests xrAnimation_converter_tests -j`
- Run suites: `ctest --test-dir ozz_utils --output-on-failure`
- Focused test loop: `ozz_utils/bin/Debug/ozz_kinematics_tests --gtest_filter=OzzKinematicsParity.*`

## Handy Tools & Scripts
- `convert_assets.sh` (repo root) -> regenerates sample `.ozz` skeletons/animations.
- `xray_to_ozz_converter` CLI handles skeleton/animation/batch conversion.
- `ozz_animation_viewer` (Debug build) can dump bind poses, JSON animation samples, and headless previews for parity checks.
- Blender snippets in `AGENT_COMMANDS.md` extract rest-pose matrices for cross-validation when needed.

## Where To Look Next
- Detailed guidance: `src/xrAnimation/AGENT_DOCS.md`
- Command recipes: `src/xrAnimation/AGENT_COMMANDS.md`
- Roadmap & priorities: `src/xrAnimation/AGENT_NEXT_STEPS.md`
- Historical context: `src/xrAnimation/CLAUDE.md`

