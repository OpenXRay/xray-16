#ifndef xrMemory_psoH
#define xrMemory_psoH
#pragma once

typedef void	__stdcall	pso_MemFill			(void* dest,  int value, u32 count);
typedef void	__stdcall	pso_MemFill32		(void* dest,  u32 value, u32 count);
typedef void	__stdcall	pso_MemCopy			(void* dest,  const void* src, u32 count);
#endif
