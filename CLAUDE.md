# Code style

- NO COMMENTS. Do not add code comments to new or edited code.

# Verification ownership

- The user owns builds, linking, shader compilation, tests, and in-game validation. Do not run these checks unless the user explicitly delegates a specific non-GUI check.
- Never launch the game or an interactive GUI application to verify renderer work.
- Never create or use throwaway scripts, numerical simulations, fixtures, or validation harnesses, including in `/tmp` or an in-process eval kernel.
- Review source without executing it and provide concrete user-owned in-game checks after behavioral changes.

# Commit messages

- One line only: `subsystem: terse imperative summary` (xrRender:, build:, memstats:, moltenvk:). No body.
- No trailers. Never add Co-Authored-By.
- Never create standalone fixup commits (build fixes, typos); amend or squash before finishing.
