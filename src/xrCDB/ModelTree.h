#pragma once

#pragma push_macro("override")
#include "OPCODE13/Opcode.h"
#pragma pop_macro("override")

namespace CDB
{
// OPCODE retains its MeshInterface pointer. Keep it with the model, not on
// the builder's stack; its strided arrays belong to the enclosing CDB::MODEL.
class ModelTree : public Opcode::Model
{
    Opcode::MeshInterface mesh;

public:
    bool Build(Fvector* vertices, u32 vertexCount, TRI* triangles, u32 triangleCount);
    bool Load(IReader* stream, u32 triangleCount, bool readCrc32 = true, bool skipCrc32 = false);
    void Save(IWriter* stream) const;
};
}
