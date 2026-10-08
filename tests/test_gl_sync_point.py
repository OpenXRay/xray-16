#!/usr/bin/env python3
"""Exercise production GL fence ownership with a recording driver."""
import pathlib
import subprocess
import tempfile

root = pathlib.Path(__file__).resolve().parents[1]
source = (root / 'src/Layers/xrRender/r__sync_point.cpp').read_text()
source = source[source.index('// Assert this just in case'):source.index('#elif defined(USE_DX11)')]
header = (root / 'src/Layers/xrRender/r__sync_point.h').read_text().replace('#pragma once', '')
with tempfile.TemporaryDirectory() as directory:
    test = pathlib.Path(directory) / 'sync.cpp'
    test.write_text(r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <set>
using u32 = unsigned; using u64 = uint64_t; using GLsync = void*;
#define RENDER_NAMESPACE test
#define ZoneScoped
#define CHK_GL(x) x
#define NODEFAULT assert(false)
void Log(const char*) { assert(false); }
constexpr unsigned GL_SYNC_GPU_COMMANDS_COMPLETE=1, GL_SYNC_FLUSH_COMMANDS_BIT=2;
constexpr unsigned GL_ALREADY_SIGNALED=3, GL_CONDITION_SATISFIED=4, GL_TIMEOUT_EXPIRED=5, GL_WAIT_FAILED=6;
constexpr bool GLAD_GL_ES_VERSION_3_0 = true;
namespace xray::render::test {
struct CHWCaps { static constexpr unsigned MAX_GPUS=8; unsigned iGPUNum=1; };
struct { CHWCaps Caps; } HW;
}
std::set<GLsync> live;
uintptr_t issued=0; unsigned waits=0;
GLsync glFenceSync(unsigned, unsigned) { auto p=reinterpret_cast<GLsync>(++issued); live.insert(p); return p; }
void glDeleteSync(GLsync p) { assert(live.erase(p)==1); }
unsigned glClientWaitSync(GLsync p, unsigned, uint64_t) {
    assert(live.count(p)); assert(reinterpret_cast<uintptr_t>(p)<issued); ++waits;
    return GL_CONDITION_SATISFIED;
}
''' + header + '\nnamespace xray::render::test {\n' + source + r'''
}
int main() {
    xray::render::test::R_sync_point sync;
    sync.Create();
    for (unsigned frame=0; frame<20; ++frame) {
        assert(sync.Wait(0,500));
        assert(waits == (frame < 2 ? 0 : frame-1));
        sync.End();
        assert(live.size()<=2);
    }
    sync.Destroy(); assert(live.empty());
    sync.Create(); assert(sync.Wait(0,500)); sync.End(); sync.Destroy(); assert(live.empty());
}
''')
    exe = pathlib.Path(directory) / 'sync'
    subprocess.run(['g++', '-std=c++17', str(test), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('GL fence latency, slot reuse and teardown passed')
