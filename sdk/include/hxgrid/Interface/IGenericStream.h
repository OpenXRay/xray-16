#ifndef IGENERICSTREAM_INCLUDED
#define IGENERICSTREAM_INCLUDED

#include "hxplatform.h"
#include "VECOM.h"

namespace VITALENGINE
{

//===========================================================
// TGENERICSTREAMCREATIONPARAMETERS
//===========================================================
typedef struct 
{
	void* data;
	DWORD length;
} TGENERICSTREAMCREATIONPARAMETERS;

//===========================================================
// IGenericStream
//===========================================================
//простой stream с интерфейсом
//может использоваться между языками в случаях, когда необходимо передавать stream
DECLARE_INTERFACE_(IGenericStream, IUnknown)
{
  IUNKNOWN_METHODS_PURE(0x762EFFFE,0x00010000)    
  
  virtual DWORD __stdcall GetVersion() const = 0;

  virtual BYTE* __stdcall GetBasePointer() = 0;
  
  virtual BYTE* __stdcall GetCurPointer() = 0;
  
  virtual bool __stdcall isReadOnly() = 0;
  
  virtual DWORD __stdcall GetLength() = 0;

  virtual void __stdcall Write(const void* Data, DWORD count) = 0;
  
  virtual DWORD __stdcall Read(void* Data, DWORD count) = 0;
   
  virtual void __stdcall Seek(DWORD pos) = 0;

  virtual DWORD __stdcall GetPos() = 0;

  virtual void __stdcall Clear() = 0;

  //do not deallocate memory
  virtual void __stdcall FastClear() = 0; 
  
  virtual void __stdcall GrowToPos(int DestSize=-1) = 0;
  
  virtual void __stdcall Skip(DWORD count) = 0;
  
  virtual void __stdcall SetLength(DWORD newLength) = 0;

  virtual void __stdcall Compact() = 0;
  
};

} //namespace

#endif IGENERICSTREAM_INCLUDED