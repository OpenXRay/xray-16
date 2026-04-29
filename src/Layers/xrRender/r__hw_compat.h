#pragma once

namespace xray::render::fg::CompatHW
{
inline bool ffp()                            { return false; }
inline bool hq_skinning()                    { return false; }
inline void shader_option_skinning(int) {}
}
