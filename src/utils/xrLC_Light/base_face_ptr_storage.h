#pragma once
#include "xrCDB/xrCDB.h"

class base_Face;

inline u32 g_base_face_index = 0;
inline xr_vector<base_Face*> g_base_face_pointers { nullptr };

inline void store_base_face_pointer(CDB::TRI& tri, base_Face* face)
{
    if constexpr (sizeof(u32) == sizeof(base_Face*))
    {
        tri.dummy = *reinterpret_cast<u32*>(&face);
        return;
    }
    const u32 idx = g_base_face_index++; // It's important to have postfix increment!
    tri.dummy = idx;

    if (g_base_face_pointers.size() < idx)
        g_base_face_pointers.resize(g_base_face_pointers.size() * 2);

    g_base_face_pointers[idx] = face;
}

inline base_Face* get_base_face_pointer(const CDB::TRI& tri)
{
    if constexpr (sizeof(u32) == sizeof(base_Face*))
    {
        return *reinterpret_cast<base_Face* const*>(&tri.dummy);
    }
    if (tri.dummy < g_base_face_pointers.size())
        return g_base_face_pointers[tri.dummy];
    return nullptr;
}
