class IReader;
class IWriter;

#include "xrCore/xrCore.h"
#include "Core/Types.hpp"

namespace XRay
{
namespace ManagedApi
{
namespace Core
{
public
ref class WriterBase abstract
{
    internal : ::IWriter* impl;

private:
    String ^ fileName;
    internal : WriterBase(::IWriter* impl);

public:
    property String ^ FileName { String ^ get(); } ~WriterBase();
    // seek
    virtual void Seek(int position) = 0;
    // tell
    virtual property int Position { int get() = 0; }
    // w
    virtual void Write(const void* buffer, int bufferSize) = 0;
    // flush
    virtual void Flush() = 0;
    // valid
    virtual property bool CanWrite { bool get(); }
    // w_u64
    void WriteUInt64(UInt64 value);
    // w_u32
    void WriteUInt32(UInt32 value);
    // w_u16
    void WriteUInt16(UInt16 value);
    // w_u8
    void WriteByte(Byte value);
    // w_s64
    void WriteInt64(Int64 value);
    // w_s32
    void WriteInt32(Int32 value);
    // w_s16
    void WriteInt16(Int16 value);
    // w_s8
    void WriteSByte(SByte value);
    // w_float
    void WriteFloat(float value);
    // w_string
    void WriteString(const char* buffer);
    // w_stringZ
    void WriteStringZ(const char* buffer);
    // w_string
    void WriteString(String ^ value);
    // w_stringZ
    void WriteStringZ(String ^ value);
    // w_fcolor
    void WriteColorF(ColorF value);
    // w_fvector4
    void WriteVector4F(Vector4F value);
    // w_fvector3
    void WriteVector3F(Vector3F value);
    // w_fvector2
    void WriteVector2F(Vector2F value);
    // w_ivector4
    void WriteVector4I(Vector4I value);
    // w_ivector3
    void WriteVector3I(Vector3I value);
    // w_ivector2
    void WriteVector2I(Vector2I value);
    // w_float_q16
    void WriteFloat16(float value, float min, float max);
    // w_float_q8
    void WriteFloat8(float value, float min, float max);
    // w_angle16
    void WriteAngle16(float value);
    // w_angle8
    void WriteAngle8(float value);
    // w_dir
    void WriteDirection(Vector3F value);
    // w_sdir
    void WriteScaledDirection(Vector3F value);
    // w_printf
    void WriteString(String ^ format, ... array<Object ^> ^ args);
    // open_chunk
    void OpenChunk(UInt32 type);
    // close_chunk
    void CloseChunk();
    // chunk_size
    property UInt32 ChunkSize { UInt32 get(); }
    // w_compressed
    void WriteCompressed(void* buffer, UInt32 bufferSize);
    // w_chunk
    void WriteChunk(UInt32 type, void* buffer, UInt32 bufferSize);
};

public
ref class ReaderBase abstract
{
    internal : ::IReader* impl;
    ReaderBase(::IReader* impl);

public:
    ~ReaderBase();
    // eof
    property bool EndOfStream { bool get(); }
    // r
    void Read(void* buffer, int byteCount);
    // r_vec3
    Vector3F ReadVector3F();
    // r_vec4
    Vector4F ReadVector4F();
    // r_u64
    UInt64 ReadUInt64();
    // r_u32
    UInt32 ReadUInt32();
    // r_u16
    UInt16 ReadUInt16();
    // r_u8
    Byte ReadByte();
    // r_s64
    Int64 ReadInt64();
    // r_s32
    Int32 ReadInt32();
    // r_s16
    Int16 ReadInt16();
    // r_s8
    SByte ReadSByte();
    // r_float
    float ReadFloat();
    // r_fvector4
    void ReadVector4F([Out] Vector4F % value);
    // r_fvector3
    void ReadVector3F([Out] Vector3F % value);
    // r_fvector2
    void ReadVector2F([Out] Vector2F % value);
    // r_ivector4(Ivector4)
    void ReadVector4I([Out] Vector4I % value);
    // r_ivector4(Ivector3)
    void ReadVector3I([Out] Vector3I % value);
    // r_ivector4(Ivector2)
    void ReadVector2I([Out] Vector2I % value);
    // r_fcolor
    void ReadColorF([Out] ColorF % value);
    // r_float_q16
    float ReadFloat16(float min, float max);
    // r_float_q8
    float ReadFloat8(float min, float max);
    // r_angle16
    float ReadAngle16();
    // r_angle8
    float ReadAngle8();
    // r_dir
    void ReadDirection([Out] Vector3F % value);
    // r_sdir
    void ReadScaledDirection([Out] Vector3F % value);
    // rewind
    void Rewind();
    // find_chunk
    UInt32 FindChunk(UInt32 id, int* isCompressed);
    // r_chunk
    bool ReadChunk(UInt32 id, void* buffer);
    // r_chunk_safe
    bool ReadChunkSafe(UInt32 id, void* buffer, UInt32 bufferSize);
    // elapsed
    property int Remain { int get(); }
    // pos
    property int Position { int get(); }
    // seek
    void Seek(int position);
    // length
    property int Length { int get(); }
    // pointer
    property void* Pointer { void* get(); }
    // advance
    void Skip(int byteCount);
    // r_string
    void ReadString(char* buffer, UInt32 bufferSize);
    void ReadString([Out] String ^ % str);
    // skip_stringZ
    void SkipStringZ();
    // r_stringZ
    void ReadStringZ(char* buffer, UInt32 bufferSize);
    void ReadStringZ([Out] String ^ % str);
    // close
    void Close();
    // open_chunk
    ReaderBase ^ OpenChunk(UInt32 id);
    // open_chunk_iterator
    ReaderBase ^ OpenChunkIterator([Out] UInt32 % id, ReaderBase ^ prevReader);
};

ref class InternalReaderBase : ReaderBase
{
    internal : InternalReaderBase(::IReader* impl) : ReaderBase(impl) {}
};
}
}
}
