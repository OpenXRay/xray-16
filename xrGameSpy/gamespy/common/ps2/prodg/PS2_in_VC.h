//
// The purpose of this file is to, as much as possible, facilitate the 
// compilation of the PS2 GNU files by the VC6 compiler.  Not all problems
// can be solved via this file.  So far, a couple of hacks are required
// directly within the headers themselves.
//

//
// This will nullify all of the GNU compiler's __attribute__ things.
//
#define __attribute__(a)

//
// This will prevent an Endian not defined error.
//
#define __IEEE_LITTLE_ENDIAN

//
// This will prevent a size_t redefinition error.
//
#define __size_t__ 

//
// This will prevent errors by headers which use u_long128.  Obviously
// the type defined below is only 32 bits, but (1) this is what the real
// headers seem to do in the GNU compiler and more importantly (2) it works!
//
typedef unsigned int u_long128;


#define long _int64

#define INLINE __inline

#define size_t int

#define __asm__ asm

#define __volatile__

#define asm(a) asm()

#define inline

#define volatile
