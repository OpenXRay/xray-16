# OPCODE 1.3 source and X-Ray integration

The implementation is vendored from `OGSR/OGSR-Engine`, revision
`01faa139995f6afdc66addb48065f053d38f397e`, directory
`ogsr_engine/xrCDB/OpCoDe`. All 98 downloaded source/documentation files were
verified against that revision's Git blob hashes. Original author notices and
the distribution ReadMe remain with the source.

The source describes OPCODE distribution 1.3 (June 2003). This OGSR variant
includes pointer-width fixes and optimized-tree data access used for caching.
It is not claimed to be a byte-for-byte copy of the unavailable
`solbjorn/OpCoDe` repository linked in OpenXRay issues #877 and #2139.

Local vendor adaptations are the build files and standalone precompiled-header
shim, shared/static-library visibility and autolink handling in Opcode.h, and
two model warning calls routed to the existing Opcode_Log hook. Engine-facing
changes are kept in xrCDB, rather than changing the library's algorithms.
The native project suppresses C5033 for the vendor's ignored legacy `register`
keywords when compiling in C++20 mode.
Two imported whitespace issues were corrected without changing statements.

The existing `Externals/OPCODE` project path builds this implementation. X-Ray
retains its own triangle ray/box/frustum tests and traversal policies. Only
node-accessor names change in those query implementations. Model construction
uses MeshInterface with a 16-byte TRI stride and a 12-byte Fvector stride;
triangle remapping is disabled, preserving IDs/materials/sectors and callbacks.
The mesh interface lives with its model. This eliminates the temporary packed
triangle-index array used by the 1.2 adapter.

Generated caches use a separate `.opcode13` suffix, leaving existing 1.2
caches intact. Their no-leaf node decoder checks sizes, checksums, finite
bounds, leaf IDs and internal offsets before installing a tree. Embedded
1.2 level.cform trees still load; an invalid embedded tree is rebuilt from
the already loaded geometry. Cache writes wait for model construction.

The standalone update is based on upstream `dev` at
`4c27925c4c500aecd090004643e9350fd7c380db`, with no Jolt prerequisite. Its
MODEL layout retains the upstream ABI. For comparisons using the existing
Jolt benchmark game DLLs, an external harness overlay inserts only the
existing benchmark branch's atomic status and thread field into MODEL to
match those DLLs. That overlay is not part of this production update.

Windows component build, with matching native dependencies and restored SDK
packages already available:

```powershell
cmake -S misc/windows/opcode -B build/opcode13 -A x64 `
  -DXRAY_NATIVE_BUILD_ROOT=C:/code/xray-windows-followup
cmake --build build/opcode13 --config Release --parallel 8
```

The tests cover the unchanged collision-query contract, random geometry,
metadata, cache roundtrips/corruption, actual serialized 1.2 nodes and embedded
legacy-tree recovery. A separate synthetic benchmark measures construction
and destruction, nearest rays, full boxes and warm cache load/destruction.
