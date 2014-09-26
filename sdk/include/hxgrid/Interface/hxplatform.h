
#pragma once

#ifdef _XBOX

#ifdef _WINDOWS_ANYWAY
#include <windows.h>

#else

#include "xtl.h"
#include "XBoxMessageBox.h"

#endif

#else _WINDOWS_ANYWAY

#define _WIN32_WINNT	0x0500

#include <windows.h>

#endif

#ifdef __cplusplus

//count указывает количество элементов типа
template<typename T> 
void __inline ChangeByteOrder(T* ptr, DWORD count = 1)
{
 char a[0]; //для такого типа конвертирование не определено
}

template<> 
void __inline ChangeByteOrder(WORD* ptr, DWORD count)
{
#ifdef _XBOX
 for (DWORD i=0; i<count; i++)
  {
   *ptr = ((((*ptr))<< 8) & 0xFF00)  |   ((((*ptr))>> 8) & 0x00FF);
    
   ptr++;
  }
#endif _XBOX
}

template<> 
void __inline ChangeByteOrder(DWORD* ptr, DWORD count)
{
#ifdef _XBOX
 for (DWORD i=0; i<count; i++)
  {
   *ptr = (((((*ptr))<<24) & 0xFF000000)  | 
    ((((*ptr))<< 8) & 0x00FF0000)  | 
    ((((*ptr))>> 8) & 0x0000FF00)  | 
    ((((*ptr))>>24) & 0x000000FF));
    
   ptr++;
  }
#endif _XBOX
}

template<> 
void __inline ChangeByteOrder(unsigned __int64* ptr, DWORD count)
{
#ifdef _XBOX
 for (DWORD i=0; i<count; i++)
  {
   *ptr = 
       ((((*ptr)<<56) & 0xFF00000000000000ULL)  | 
        (((*ptr)<<40) & 0x00FF000000000000ULL)  | 
        (((*ptr)<<24) & 0x0000FF0000000000ULL)  | 
        (((*ptr)<< 8) & 0x000000FF00000000ULL)  | 
        (((*ptr)>> 8) & 0x00000000FF000000ULL)  | 
        (((*ptr)>>24) & 0x0000000000FF0000ULL)  | 
        (((*ptr)>>40) & 0x000000000000FF00ULL)  | 
        (((*ptr)>>56) & 0x00000000000000FFULL));
    
   ptr++;
  }
#endif _XBOX
}


template<> 
void __inline ChangeByteOrder(int* ptr, DWORD count)
{
 ChangeByteOrder((DWORD*)ptr, count);
}

template<> 
void __inline ChangeByteOrder(long* ptr, DWORD count)
{
 ChangeByteOrder((DWORD*)ptr, count);
}


template<> 
void __inline ChangeByteOrder(short* ptr, DWORD count)
{
 ChangeByteOrder((WORD*)ptr, count);
}

template<> 
void __inline ChangeByteOrder(__int64* ptr, DWORD count)
{
 ChangeByteOrder((unsigned __int64*)ptr, count);
}


template<> 
void __inline ChangeByteOrder(double* ptr, DWORD count)
{
 ChangeByteOrder((unsigned __int64*)ptr, count);
}

template<> 
void __inline ChangeByteOrder(float* ptr, DWORD count)
{
 ChangeByteOrder((DWORD*)ptr, count);
}

#ifdef _XBOX
void __inline MessageBeep(DWORD d)
{
}
#endif _XBOX

#endif __cplusplus