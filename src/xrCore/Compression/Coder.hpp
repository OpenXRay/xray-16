/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Contents: 'Carryless rangecoder' by Dmitry Subbotin                     *
 *  Comments: this implementation is claimed to be a public domain          *
 ****************************************************************************/
/**********************  Original text  *************************************
////////   Carryless rangecoder (c) 1999 by Dmitry Subbotin   ////////

typedef unsigned int  uint;
typedef unsigned char uc;

#define  DO(n)     for (int _=0; _<n; _++)
#define  TOP       (1<<24)
#define  BOT       (1<<16)


class RangeCoder
{
 uint  low, code, range, passed;
 FILE  *f;

 void OutByte (uc c)           { passed++; fputc(c,f); }
 uc   InByte ()                { passed++; return fgetc(f); }

public:

 uint GetPassed ()             { return passed; }
 void StartEncode (FILE *F)    { f=F; passed=low=0;  range= (uint) -1; }
 void FinishEncode ()          { DO(4)  OutByte(low>>24), low<<=8; }
 void StartDecode (FILE *F)    { passed=low=code=0;  range= (uint) -1;
                                 f=F; DO(4) code= code<<8 | InByte();
                               }

 void Encode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq * (range/= totFreq);
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       OutByte(low>>24), range<<=8, low<<=8;
 }

 uint GetFreq (uint totFreq) {
   uint tmp= (code-low) / (range/= totFreq);
   if (tmp >= totFreq)  throw ("Input data corrupt"); // or force it to return
   return tmp;                                         // a valid value :)
 }

 void Decode (uint cumFreq, uint freq, uint totFreq) {
    assert(cumFreq+freq<totFreq && freq && totFreq<=BOT);
    low  += cumFreq*range;
    range*= freq;
    while ((low ^ low+range)<TOP || range<BOT && ((range= -low & BOT-1),1))
       code= code<<8 | InByte(), range<<=8, low<<=8;
 }
};
*****************************************************************************/
namespace ppmd
{
enum
{
    TOP = 1 << 24,
    BOT = 1 << 15
};

struct SUBRANGE
{
    DWORD low, high, scale;
};

static SUBRANGE SubRange = {0, 0, 0};
static DWORD low = 0, code = 0, range = 0;

inline void rcEncNormalize(_PPMD_FILE* stream)
{
    while ((low ^ (low + range)) < TOP || range < BOT && ((range = -low & (BOT - 1)), 1))
    {
        _PPMD_E_PUTC(low >> 24, stream);
        range <<= 8;
        low <<= 8;
    }
}

static inline void rcInitEncoder()
{
    low = 0;
    range = DWORD(-1);
}
/*
#define RC_ENC_NORMALIZE(stream) {                                          \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        _PPMD_E_PUTC(low >> 24,stream);                                     \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
*/
static inline void rcEncodeSymbol()
{
    low += SubRange.low * (range /= SubRange.scale);
    range *= SubRange.high - SubRange.low;
}

static inline void rcFlushEncoder(_PPMD_FILE* stream)
{
    for (UINT i = 0; i < 4; i++)
    {
        _PPMD_E_PUTC(low >> 24, stream);
        low <<= 8;
    }
}
static inline void rcInitDecoder(_PPMD_FILE* stream)
{
    low = code = 0;
    range = DWORD(-1);
    for (UINT i = 0; i < 4; i++)
        code = (code << 8) | _PPMD_D_GETC(stream);
}

inline void rcDecNormalize(_PPMD_FILE* stream)
{
    while ((low ^ (low + range)) < TOP || range < BOT && ((range = -low & (BOT - 1)), 1))
    {
        code = (code << 8) | _PPMD_D_GETC(stream);
        range <<= 8;
        low <<= 8;
    }
}

/*
#define RC_DEC_NORMALIZE(stream) {                                          \
    while ((low ^ (low+range)) < TOP || range < BOT &&                      \
            ((range= -low & (BOT-1)),1)) {                                  \
        code=(code << 8) | _PPMD_D_GETC(stream);                            \
        range <<= 8;                        low <<= 8;                      \
    }                                                                       \
}
*/

static inline UINT rcGetCurrentCount() { return (code - low) / (range /= SubRange.scale); }
static inline void rcRemoveSubrange()
{
    low += range * SubRange.low;
    range *= SubRange.high - SubRange.low;
}

static inline UINT rcBinStart(UINT f0, UINT Shift) { return f0 * (range >>= Shift); }
static inline UINT rcBinDecode(UINT tmp) { return (code - low >= tmp); }
static inline void rcBinCorrect0(UINT tmp) { range = tmp; }
static inline void rcBinCorrect1(UINT tmp, UINT f1)
{
    low += tmp;
    range *= f1;
}

} // namespace ppmd
