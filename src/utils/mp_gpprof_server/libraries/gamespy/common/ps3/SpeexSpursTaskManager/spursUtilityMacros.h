#ifndef __CELL_UTILITY_MACROS_H
#define __CELL_UTILITY_MACROS_H

// This type pretends to be a pointer to a type on the PPU, and
// just an unsigned int on the SPU.
#ifndef CELL_PPU_POINTER
#define CELL_PPU_PTR_TYPE uint32_t

#ifdef __SPU__
#define CELL_PPU_POINTER(x) CELL_PPU_PTR_TYPE
#else // __SPU__
#define CELL_PPU_POINTER(x) x *

// Hope we never switch away from 32 bits...
// But this is here just in case...
//NX_COMPILE_TIME_ASSERT(sizeof(void *)==sizeof(CELL_PPU_PTR_TYPE));
#endif // __SPU__
#endif // CELL_PPU_POINTER

#endif
// end  __CELL_UTILITY_MACROS_H
