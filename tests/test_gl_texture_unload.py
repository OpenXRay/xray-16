"""Exercise the production unload method with GL deletion calls recorded."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class TextureUnloadTest(unittest.TestCase):
    def test_deleted_handles_cannot_be_reused_or_deleted_twice(self):
        source = (ROOT / 'src/Layers/xrRenderGL/glSH_Texture.cpp').read_text()
        start = source.index('void CTexture::Unload()')
        end = source.index('void CTexture::desc_update()', start)
        unload = source[start:end]
        prefix = r'''
#include <cassert>
#include <vector>
using u32 = unsigned;
#define ZoneScoped
#define FALSE false
#define CHK_GL(x) x
struct CBackend {};
namespace fastdelegate {
template<class A, class B> struct FastDelegate2 {
    FastDelegate2() = default;
    template<class Owner> FastDelegate2(Owner*, void (Owner::*)(A, B)) {}
};
}
template<class T> void xr_delete(T*& value) { delete value; value = nullptr; }
std::vector<unsigned> textures, buffers;
void glDeleteTextures(unsigned count, const unsigned* ids) {
    for (unsigned i = 0; i < count; ++i) if (ids[i]) textures.push_back(ids[i]);
}
void glDeleteBuffers(unsigned count, const unsigned* ids) {
    for (unsigned i = 0; i < count; ++i) if (ids[i]) buffers.push_back(ids[i]);
}
struct CTexture {
    struct { bool bLoaded = true; } flags;
    unsigned pSurface = 41, pBuffer = 71, desc_cache = 41;
    int m_width = 1024, m_height = 1024;
    int* pTheora = nullptr;
    std::vector<unsigned> seqDATA;
    fastdelegate::FastDelegate2<CBackend&, u32> bind;
    void apply_load(CBackend&, u32) {}
    void Unload();
};
'''
        check = r'''
int main() {
    CTexture texture;
    texture.Unload();
    assert(!texture.flags.bLoaded);
    assert(texture.pSurface == 0 && texture.pBuffer == 0 && texture.desc_cache == 0);
    assert(texture.m_width == 0 && texture.m_height == 0);
    assert((textures == std::vector<unsigned>{41}));
    assert((buffers == std::vector<unsigned>{71}));
    texture.Unload();
    assert(textures.size() == 1 && buffers.size() == 1);
    texture.pSurface = 52;
    texture.seqDATA = {51, 52};
    texture.Unload();
    assert((textures == std::vector<unsigned>{41, 51, 52}));
    assert(texture.seqDATA.empty() && texture.pSurface == 0);
}
'''
        with tempfile.TemporaryDirectory() as directory:
            cpp = Path(directory) / 'unload.cpp'
            executable = Path(directory) / 'unload'
            cpp.write_text(prefix + unload + check)
            subprocess.run(['g++', '-std=c++17', '-O2', str(cpp), '-o', str(executable)], check=True)
            subprocess.run([str(executable)], check=True)


if __name__ == '__main__':
    unittest.main()
