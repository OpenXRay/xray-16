#pragma once

#include <cstdlib>

#ifndef WIN32
#define __stdcall
#endif

void* __stdcall xr_aligned_malloc(size_t, size_t);
void* __stdcall xr_aligned_offset_malloc(size_t, size_t, size_t);
void* __stdcall xr_aligned_realloc(void*, size_t, size_t);
void* __stdcall xr_aligned_offset_realloc(void*, size_t, size_t, size_t);
void __stdcall xr_aligned_free(void*);
size_t __stdcall xr_aligned_msize(void*);
