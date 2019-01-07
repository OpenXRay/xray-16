#pragma once

size_t __stdcall xr_aligned_msize(void*);
void __stdcall xr_aligned_free(void*);
void* __stdcall xr_aligned_malloc(size_t, size_t);
void* __stdcall xr_aligned_offset_malloc(size_t, size_t, size_t);
void* __stdcall xr_aligned_realloc(void*, size_t, size_t);
void* __stdcall xr_aligned_offset_realloc(void*, size_t, size_t, size_t);
