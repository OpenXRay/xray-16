#include "Pch.hpp"
#include "FS.hpp"
#include "xrCore/xrCore.h"
#pragma warning(push)
#pragma warning(disable : 4995) // ignore deprecation warnings
#include <msclr/marshal.h>
#pragma warning(pop)

using namespace msclr::interop;
using namespace System::Diagnostics;

namespace XRay
{
namespace ManagedApi
{
namespace Core
{
WriterBase::WriterBase(::IWriter* impl) { this->impl = impl; }
WriterBase::~WriterBase()
{
    delete impl;
    impl = nullptr;
}
String ^ WriterBase::FileName::get()
{
    if (!fileName)
        fileName = gcnew String(impl->fName.c_str());
    return fileName;
}
void WriterBase::WriteUInt64(UInt64 value) { impl->w_u64(value); }
void WriterBase::WriteUInt32(UInt32 value) { impl->w_u32(value); }
void WriterBase::WriteUInt16(UInt16 value) { impl->w_u16(value); }
void WriterBase::WriteByte(Byte value) { impl->w_u8(value); }
void WriterBase::WriteInt64(Int64 value) { impl->w_s64(value); }
void WriterBase::WriteInt32(Int32 value) { impl->w_s32(value); }
void WriterBase::WriteInt16(Int16 value) { impl->w_s16(value); }
void WriterBase::WriteSByte(SByte value) { impl->w_s8(value); }
void WriterBase::WriteFloat(float value) { impl->w_float(value); }
void WriterBase::WriteString(const char* buffer) { impl->w_string(buffer); }
void WriterBase::WriteStringZ(const char* buffer) { impl->w_stringZ(buffer); }
void WriterBase::WriteString(String ^ value)
{
    marshal_context context;
    auto tmpStr = context.marshal_as<const char*>(value);
    impl->w_string(tmpStr);
}
void WriterBase::WriteStringZ(String ^ value)
{
    marshal_context context;
    auto tmpStr = context.marshal_as<const char*>(value);
    impl->w_stringZ(tmpStr);
}
void WriterBase::WriteColorF(ColorF value) { impl->w_fcolor((const Fcolor&)value); }
void WriterBase::WriteVector4F(Vector4F value) { impl->w_fvector4((const Fvector4&)value); }
void WriterBase::WriteVector3F(Vector3F value) { impl->w_fvector3((const Fvector3&)value); }
void WriterBase::WriteVector2F(Vector2F value) { impl->w_fvector2((const Fvector2&)value); }
void WriterBase::WriteVector4I(Vector4I value) { impl->w_ivector4((const Ivector4&)value); }
void WriterBase::WriteVector3I(Vector3I value) { impl->w_ivector3((const Ivector3&)value); }
void WriterBase::WriteVector2I(Vector2I value) { impl->w_ivector2((const Ivector2&)value); }
void WriterBase::WriteFloat16(float value, float min, float max) { impl->w_float_q16(value, min, max); }
void WriterBase::WriteFloat8(float value, float min, float max) { impl->w_float_q8(value, min, max); }
void WriterBase::WriteAngle16(float value) { impl->w_angle16(value); }
void WriterBase::WriteAngle8(float value) { impl->w_angle8(value); }
void WriterBase::WriteDirection(Vector3F value) { impl->w_dir((const Fvector3&)value); }
void WriterBase::WriteScaledDirection(Vector3F value) { impl->w_sdir((const Fvector3&)value); }
void WriterBase::WriteString(String ^ format, ... array<Object ^> ^ args)
{
    auto str = String::Format(format, args);
    marshal_context context;
    auto tmpStr = context.marshal_as<const char*>(str);
    impl->w(tmpStr, xr_strlen(tmpStr));
}
void WriterBase::OpenChunk(UInt32 type) { impl->open_chunk(type); }
void WriterBase::CloseChunk() { impl->close_chunk(); }
UInt32 WriterBase::ChunkSize::get() { return impl->chunk_size(); }
void WriterBase::WriteCompressed(void* buffer, UInt32 bufferSize) { impl->w_compressed(buffer, bufferSize); }
void WriterBase::WriteChunk(UInt32 type, void* buffer, UInt32 bufferSize) { impl->w_chunk(type, buffer, bufferSize); }
bool WriterBase::CanWrite::get() { return impl->valid(); }
ReaderBase::~ReaderBase()
{
    delete impl;
    impl = nullptr;
}
ReaderBase::ReaderBase(::IReader* impl) { this->impl = impl; }
bool ReaderBase::EndOfStream::get() { return !!impl->eof(); }
void ReaderBase::Read(void* buffer, int byteCount) { impl->r(buffer, byteCount); }
Vector3F ReaderBase::ReadVector3F() { return *(Vector3F*)&impl->r_vec3(); }
Vector4F ReaderBase::ReadVector4F() { return *(Vector4F*)&impl->r_vec4(); }
UInt64 ReaderBase::ReadUInt64() { return impl->r_u64(); }
UInt32 ReaderBase::ReadUInt32() { return impl->r_u32(); }
UInt16 ReaderBase::ReadUInt16() { return impl->r_u16(); }
Byte ReaderBase::ReadByte() { return impl->r_u8(); }
Int64 ReaderBase::ReadInt64() { return impl->r_s64(); }
Int32 ReaderBase::ReadInt32() { return impl->r_s32(); }
Int16 ReaderBase::ReadInt16() { return impl->r_s16(); }
SByte ReaderBase::ReadSByte() { return impl->r_s8(); }
float ReaderBase::ReadFloat() { return impl->r_float(); }
void ReaderBase::ReadVector4F([Out] Vector4F % v)
{
    Fvector4 tmp;
    impl->r_fvector4(tmp);
    v = reinterpret_cast<Vector4F&>(tmp);
}
void ReaderBase::ReadVector3F([Out] Vector3F % v)
{
    Fvector3 tmp;
    impl->r_fvector3(tmp);
    v = reinterpret_cast<Vector3F&>(tmp);
}
void ReaderBase::ReadVector2F([Out] Vector2F % v)
{
    Fvector2 tmp;
    impl->r_fvector2(tmp);
    v = reinterpret_cast<Vector2F&>(tmp);
}
void ReaderBase::ReadVector4I([Out] Vector4I % v)
{
    Ivector4 tmp;
    impl->r_ivector4(tmp);
    v = reinterpret_cast<Vector4I&>(tmp);
}
void ReaderBase::ReadVector3I([Out] Vector3I % v)
{
    Ivector3 tmp;
    impl->r_ivector4(tmp);
    v = reinterpret_cast<Vector3I&>(tmp);
}
void ReaderBase::ReadVector2I([Out] Vector2I % v)
{
    Ivector2 tmp;
    impl->r_ivector4(tmp);
    v = reinterpret_cast<Vector2I&>(tmp);
}
void ReaderBase::ReadColorF([Out] ColorF % v)
{
    Fcolor tmp;
    impl->r_fcolor(tmp);
    v = reinterpret_cast<ColorF&>(tmp);
}
float ReaderBase::ReadFloat16(float min, float max) { return impl->r_float_q16(min, max); }
float ReaderBase::ReadFloat8(float min, float max) { return impl->r_float_q8(min, max); }
float ReaderBase::ReadAngle16() { return impl->r_angle16(); }
float ReaderBase::ReadAngle8() { return impl->r_angle8(); }
void ReaderBase::ReadDirection([Out] Vector3F % A)
{
    Fvector3 tmp;
    impl->r_dir(tmp);
    A = reinterpret_cast<Vector3F&>(tmp);
}
void ReaderBase::ReadScaledDirection([Out] Vector3F % A)
{
    Fvector3 tmp;
    impl->r_sdir(tmp);
    A = reinterpret_cast<Vector3F&>(tmp);
}
void ReaderBase::Rewind() { impl->rewind(); }
UInt32 ReaderBase::FindChunk(UInt32 id, int* isCompressed) { return impl->find_chunk(id, isCompressed); }
bool ReaderBase::ReadChunk(UInt32 id, void* buffer) { return !!impl->r_chunk(id, buffer); }
bool ReaderBase::ReadChunkSafe(UInt32 id, void* buffer, UInt32 bufferSize)
{
    return !!impl->r_chunk_safe(id, buffer, bufferSize);
}
int ReaderBase::Remain::get() { return impl->elapsed(); }
int ReaderBase::Position::get() { return impl->tell(); }
void ReaderBase::Seek(int position) { impl->seek(position); }
int ReaderBase::Length::get() { return impl->length(); }
void* ReaderBase::Pointer::get() { return impl->pointer(); }
void ReaderBase::Skip(int byteCount) { impl->advance(byteCount); }
void ReaderBase::ReadString(char* buffer, UInt32 bufferSize) { impl->r_string(buffer, bufferSize); }
void ReaderBase::ReadString([Out] String ^ % str)
{
    xr_string tmp;
    impl->r_string(tmp);
    str = gcnew String(tmp.c_str());
}
void ReaderBase::SkipStringZ() { impl->skip_stringZ(); }
void ReaderBase::ReadStringZ(char* buffer, UInt32 bufferSize) { impl->r_stringZ(buffer, bufferSize); }
void ReaderBase::ReadStringZ([Out] String ^ % str)
{
    xr_string tmp;
    impl->r_stringZ(tmp);
    str = gcnew String(tmp.c_str());
}
void ReaderBase::Close()
{
    impl->close(); // close deletes impl
    impl = nullptr;
}
ReaderBase ^ ReaderBase::OpenChunk(UInt32 id)
{
    auto rawReader = impl->open_chunk(id);
    return gcnew InternalReaderBase(rawReader);
}
ReaderBase ^ ReaderBase::OpenChunkIterator([Out] UInt32 % id, ReaderBase ^ prevReader)
{
    UInt32 tmpId;
    auto rawReader = impl->open_chunk_iterator(tmpId, prevReader ? prevReader->impl : nullptr);
    id = tmpId;
    if (prevReader)
        prevReader->impl = nullptr; // open_chunk_iterator deletes impl
    return gcnew InternalReaderBase(rawReader);
}
}
}
}
