# Diplomatic Pacification Mode

## Summary

This branch introduces a Diplomatic Pacification Mode that changes the default gameplay flow from violent combat to hologram-based de-escalation.

The implementation is intentionally conservative:
- default to Diplomatic mode
- allow user opt-in to Original mode
- show a warning if Original mode is selected
- keep violent content disabled by default

## Core concept

- All default weapons are replaced with hologram projector props in Diplomatic mode.
- The player interacts with a Blender-like design terminal instead of firing bullets.
- Targets are pacified by generating a matched hologram pattern.
- NPCs use inference engines to generate counter-holograms with translucent, AI-like visual artifacts.
- A pacification meter replaces health damage as the primary conflict metric.

## Files added

- `src/xrGame/GameplayMode/GameplayMode.h`
- `src/xrGame/GameplayMode/GameplayMode.cpp`
- `src/xrGame/GameplayMode/HologramProjector.h`
- `src/xrGame/GameplayMode/HologramProjector.cpp`
- `src/xrGame/GameplayMode/DiplomaticModeConfig.ini`

## Default behavior

When the game starts:
- `EGameplayMode::Diplomatic` is selected unless the user opts into Original mode.
- Violent content is not loaded by default.
- Hologram projectors are enabled by default.

## Original mode warning

If a user intentionally chooses Original mode, the warning should read:

> The original gameplay includes violent content which can affect child development. Suggestion: use the diplomatic mode.

## Notes

This is a safe, source-level framework for gradual integration into the engine. It is not a complete engine patch, but it provides the scaffolding and logic structure necessary for upstream review.
