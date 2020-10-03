/****************************************************************************
 *  Contents: program for the building of static PPM tree                   *
 *  (C) 2003 by Dmitry Shkarin, e-mail: dmitry.shkarin@mtu-net.ru           *
 *  Comments: It is approximate solution. Algorithm for the building of     *
 * globally optimal tree is unknown, algorithm for the building of locally  *
 * optimal tree is too complex and memory demanding.                        *
 ****************************************************************************/
#include <math.h>
#include <stdio.h>
#define NOMINMAX
#include <windows.h>
#define XR_PLATFORM_WINDOWS
#include "xrCore/_types.h"
#pragma hdrstop
#include "PPMT_SA.hpp"

const int MAX_O = 8;
const int UP_FREQ = 5, INT_BITS = 7, PERIOD_BITS = 7, TOT_BITS = INT_BITS + PERIOD_BITS, INTERVAL = 1 << INT_BITS,
          BIN_SCALE = 1 << TOT_BITS, MAX_FREQ = 124;

#pragma pack(1)
static struct PPM_CONTEXT
{
    u32 EscFreq;
    u16 NumStats, Dummy;
    u32 SummFreq;
    struct STATE
    {
        u8 Symbol, Flag;
        u32 Freq;
        PPM_CONTEXT* Successor;
    } * Stats;
    PPM_CONTEXT* Suffix;
    inline void encodeBinSymbol(int symbol);
    inline void encodeSymbol(int symbol);
    PPM_CONTEXT* cutOff(int o, int ob, double b);
    void clean(int o, BOOL FreqsOnly);
    void write(int o, FILE* fp);
    void write(int o, FILE* fp1, FILE* fp2, FILE* fp3);
    inline PPM_CONTEXT* createChild(STATE* pStats, STATE& FirstState);
    STATE& oneState() const { return (STATE&)Dummy; }
} * MinContext, *MaxContext;
#pragma pack()

static u8 QTable[260]; // constants
static PPM_CONTEXT::STATE* FoundState; // found next state transition
static int NumMasked, OrderFall, MaxOrder;
static u8 CharMask[256];

inline PPM_CONTEXT* PPM_CONTEXT::createChild(STATE* pStats, STATE& FirstState)
{
    PPM_CONTEXT* pc = (PPM_CONTEXT*)AllocContext();
    if (pc)
    {
        memset(pc, 0, sizeof(PPM_CONTEXT));
        pc->oneState() = FirstState;
        pc->Suffix = this;
        pStats->Successor = pc;
    }
    return pc;
}
static void _FASTCALL StartModelRare(int MaxOrder)
{
    int i, k, m, Step;
    ::MaxOrder = OrderFall = MaxOrder;
    InitSubAllocator();
    MinContext = MaxContext = (PPM_CONTEXT*)AllocContext();
    MinContext->Suffix = NULL;
    MinContext->NumStats = 255;
    FoundState = MinContext->Stats = (PPM_CONTEXT::STATE*)AllocUnits(256 / 2);
    for (i = MinContext->SummFreq = MinContext->EscFreq = 0; i < 256; i++)
    {
        MinContext->Stats[i].Symbol = i;
        MinContext->Stats[i].Freq = 0;
        MinContext->Stats[i].Successor = NULL;
    }
    for (i = 0; i < UP_FREQ; i++)
        QTable[i] = i;
    for (m = i = UP_FREQ, k = Step = 1; i < 260; i++)
    {
        QTable[i] = m;
        if (!--k)
        {
            k = ++Step;
            m++;
        }
    }
}
static inline PPM_CONTEXT* CreateSuccessors(BOOL Skip, PPM_CONTEXT::STATE* p1)
{
    // static UpState declaration bypasses IntelC bug
    static PPM_CONTEXT::STATE UpState;
    PPM_CONTEXT *pc = MinContext, *UpBranch = FoundState->Successor;
    PPM_CONTEXT::STATE *p, *ps[MAX_O], **pps = ps;
    if (!Skip)
    {
        *pps++ = FoundState;
        if (!pc->Suffix)
            goto NO_LOOP;
    }
    if (p1)
    {
        p = p1;
        pc = pc->Suffix;
        goto LOOP_ENTRY;
    }
    do
    {
        pc = pc->Suffix;
        if (pc->NumStats)
        {
            if ((p = pc->Stats)->Symbol != FoundState->Symbol)
                do
                {
                    p++;
                } while (p->Symbol != FoundState->Symbol);
        }
        else
            p = &(pc->oneState());
    LOOP_ENTRY:
        if (p->Successor != UpBranch)
        {
            pc = p->Successor;
            break;
        }
        *pps++ = p;
    } while (pc->Suffix);
NO_LOOP:
    if (pps == ps)
        return pc;
    UpState.Symbol = *(u8*)UpBranch;
    UpState.Freq = UpState.Flag = 0;
    UpState.Successor = (PPM_CONTEXT*)(((u8*)UpBranch) + 1);
    do
    {
        pc = pc->createChild(*--pps, UpState);
        if (!pc)
            return NULL;
    } while (pps != ps);
    return pc;
}
static inline void UpdateModel()
{
    static PPM_CONTEXT::STATE fs;
    PPM_CONTEXT::STATE* p = NULL;
    fs = *FoundState;
    PPM_CONTEXT *pc, *Successor;
    u32 ns1;
    if (!OrderFall)
    {
        MinContext = MaxContext = FoundState->Successor = CreateSuccessors(TRUE, p);
        if (!MinContext)
            goto RESTART_MODEL;
        return;
    }
    *pText++ = fs.Symbol;
    Successor = (PPM_CONTEXT*)pText;
    if (pText >= UnitsStart)
        goto RESTART_MODEL;
    if (fs.Successor)
    {
        if ((u8*)fs.Successor <= pText && (fs.Successor = CreateSuccessors(FALSE, p)) == NULL)
            goto RESTART_MODEL;
        if (!--OrderFall)
        {
            Successor = fs.Successor;
            pText -= (MaxContext != MinContext);
        }
    }
    else
    {
        FoundState->Successor = Successor;
        fs.Successor = MinContext;
    }
    for (pc = MaxContext; pc != MinContext; pc = pc->Suffix)
    {
        if ((ns1 = pc->NumStats) != 0)
        {
            if ((ns1 & 1) != 0)
            {
                pc->Stats = (PPM_CONTEXT::STATE*)ExpandUnits(pc->Stats, (ns1 + 1) >> 1);
                if (!pc->Stats)
                    goto RESTART_MODEL;
            }
        }
        else
        {
            p = (PPM_CONTEXT::STATE*)AllocUnits(1);
            if (!p)
                goto RESTART_MODEL;
            *p = pc->oneState();
            pc->Stats = p;
            pc->SummFreq = p->Freq;
            pc->Dummy = 0;
        }
        p = pc->Stats + (++pc->NumStats);
        p->Successor = Successor;
        p->Symbol = fs.Symbol;
        p->Freq = p->Flag = 0;
    }
    MaxContext = MinContext = fs.Successor;
    return;
RESTART_MODEL:
    printf("Out of memory!");
    exit(-1);
}
inline void PPM_CONTEXT::encodeBinSymbol(int symbol)
{
    STATE& rs = oneState();
    if (rs.Symbol == symbol)
        (FoundState = &rs)->Freq++;
    else
    {
        NumMasked = 0;
        FoundState = NULL;
    }
}
inline void PPM_CONTEXT::encodeSymbol(int symbol)
{
    STATE* p = Stats;
    for (int i = NumStats; p->Symbol != symbol; p++)
        if (--i < 0)
        {
            NumMasked = NumStats;
            FoundState = NULL;
            return;
        }
    (FoundState = p)->Freq++;
    SummFreq++;
    while (p != Stats && p[0].Freq > p[-1].Freq)
    {
        SWAP(p[0], p[-1]);
        FoundState = --p;
    }
}
static void EncodeFile2(FILE* DecodedFile)
{
    while (MaxContext->Suffix)
        MaxContext = MaxContext->Suffix;
    MaxContext->clean(0, TRUE);
    rewind(DecodedFile);
    MinContext = MaxContext;
    for (int ns = MinContext->NumStats, NumBytes = 0;; ns = MinContext->NumStats)
    {
        int c = getc(DecodedFile);
        if (ns)
            MinContext->encodeSymbol(c);
        else
            MinContext->encodeBinSymbol(c);
        while (!FoundState)
        {
            do
            {
                MinContext->EscFreq++;
                MinContext = MinContext->Suffix;
                if (!MinContext)
                    goto STOP_ENCODING;
            } while (MinContext->NumStats == NumMasked);
            MinContext->encodeSymbol(c);
        }
        if (FoundState->Successor)
            MinContext = FoundState->Successor;
        else
            while (MinContext->Suffix != NULL)
            {
                MinContext = MinContext->Suffix;
                if (!MinContext->NumStats)
                    FoundState = &(MinContext->oneState());
                else
                    for (FoundState = MinContext->Stats; FoundState->Symbol != c; FoundState++)
                        ;
                if (FoundState->Successor)
                {
                    MinContext = FoundState->Successor;
                    break;
                }
            }
        MaxContext = MinContext;
        if (((++NumBytes) & 0x7FFFF) == 0)
            PrintInfo(DecodedFile);
    }
STOP_ENCODING:
    PrintInfo(DecodedFile);
    printf("\n");
    while (MaxContext->Suffix)
        MaxContext = MaxContext->Suffix;
    MaxContext->clean(0, FALSE);
}
static void EncodeFile1(FILE* DecodedFile)
{
    for (int ns = MinContext->NumStats, NumBytes = 0;; ns = MinContext->NumStats)
    {
        int c = getc(DecodedFile);
        if (ns)
            MinContext->encodeSymbol(c);
        else
            MinContext->encodeBinSymbol(c);
        while (!FoundState)
        {
            do
            {
                OrderFall++;
                MinContext = MinContext->Suffix;
                if (!MinContext)
                    goto STOP_ENCODING;
            } while (MinContext->NumStats == NumMasked);
            MinContext->encodeSymbol(c);
        }
        if (!OrderFall && (u8*)FoundState->Successor > pText)
            MinContext = MaxContext = FoundState->Successor;
        else if (UnitsStart - pText > 128 * UNIT_SIZE)
            UpdateModel();
        else
            break;
        if (((++NumBytes) & 0x3FFFF) == 0)
            PrintInfo(DecodedFile);
    }
STOP_ENCODING:
    PrintInfo(DecodedFile);
    printf("\n");
    EncodeFile2(DecodedFile);
}
void PPM_CONTEXT::clean(int o, BOOL FreqsOnly)
{
    if (FreqsOnly)
        EscFreq = SummFreq = 0;
    if (!NumStats)
    {
        if (o != MaxOrder && (u8*)oneState().Successor >= UnitsStart)
            oneState().Successor->clean(o + 1, FreqsOnly);
        else if ((u8*)oneState().Successor < UnitsStart || !FreqsOnly)
            oneState().Successor = NULL;
    }
    else
        for (STATE* p = Stats; p <= Stats + NumStats; p++)
        {
            if (FreqsOnly)
                p->Freq = 0;
            if (o != MaxOrder && (u8*)p->Successor >= UnitsStart)
                p->Successor->clean(o + 1, FreqsOnly);
            else if ((u8*)p->Successor < UnitsStart || !FreqsOnly)
                p->Successor = NULL;
        }
}

static const double MULT = 1.0 / log(2.0), COD_ERR = 0.02;
inline double GetExtraBits(double f, double e, double f1, double sf1, double sf0, int ns)
{
    double ExtraBits = f * log(f * (sf1 + f1 + f) / ((e + f) * double(f1 + f)));
    if (e)
        ExtraBits += e * log(e * (sf1 + f1 + f) / (sf1 * (e + f)));
    if (sf0 != e)
        ExtraBits += (sf0 - e) * log((sf0 + f) / sf0);
    if (f1)
        ExtraBits -= f1 * log((f1 + f) / f1);
    ExtraBits *= MULT;
    if (ns)
        ExtraBits += COD_ERR * f;
    else
        ExtraBits -= COD_ERR * e;
    return ExtraBits;
}
static u32 SizeOfModel, nc;
PPM_CONTEXT* PPM_CONTEXT::cutOff(int o, int ob, double b)
{
    STATE tmp, *p, *p1;
    int ns = NumStats;
    double sf0, sf1, ExtraBits;
    if (o < ob)
    {
        if (!ns)
        {
            if ((p = &oneState())->Successor)
                p->Successor = p->Successor->cutOff(o + 1, ob, b);
        }
        else
            for (p = Stats + ns; p >= Stats; p--)
                if (p->Successor)
                    p->Successor = p->Successor->cutOff(o + 1, ob, b);
        return this;
    }
    else if (!ns)
    {
        p = &oneState();
        if (!Suffix->NumStats)
        {
            sf0 = (p1 = &(Suffix->oneState()))->Freq + (sf1 = Suffix->EscFreq);
        }
        else
        {
            for (p1 = Suffix->Stats; p->Symbol != p1->Symbol; p1++)
                ;
            sf0 = Suffix->SummFreq + Suffix->EscFreq;
            sf1 = sf0 - p1->Freq;
        }
        if (!p->Successor && !p->Flag)
        {
            ExtraBits = GetExtraBits(p->Freq, EscFreq, p1->Freq, sf1, sf0, 0);
            if (ExtraBits < b || 4 * p->Freq < EscFreq)
            {
                if (!Suffix->NumStats)
                    Suffix->oneState().Freq += p->Freq;
                else
                {
                    p1->Freq += p->Freq;
                    Suffix->SummFreq += p->Freq;
                }
                return NULL;
            }
        }
        nc++;
        SizeOfModel += 3;
        p->Flag = 0;
        p1->Flag = 1;
        return this;
    }
    for (p = Stats + ns; p >= Stats; p--)
        CharMask[p->Symbol] = 1;
    sf0 = Suffix->SummFreq + (sf1 = Suffix->EscFreq);
    for (p1 = Suffix->Stats + Suffix->NumStats; p1 >= Suffix->Stats; p1--)
        if (!CharMask[p1->Symbol])
            sf1 += p1->Freq;
    for (p = Stats + ns; p >= Stats; p--)
    {
        CharMask[p->Symbol] = 0;
        if (p->Flag || p->Successor)
            continue;
        for (p1 = Suffix->Stats; p->Symbol != p1->Symbol; p1++)
            ;
        ExtraBits = GetExtraBits(p->Freq, EscFreq, p1->Freq, sf1, sf0, ns);
        if (ExtraBits < b || 64 * p->Freq * (ns + 1) < SummFreq)
        {
            SummFreq -= p->Freq;
            EscFreq += p->Freq;
            p1->Freq += p->Freq;
            Suffix->SummFreq += p->Freq;
            sf1 += p1->Freq;
            sf0 += p->Freq;
            for (p1 = p; p1 != Stats + ns; p1++)
                SWAP(p1[0], p1[1]);
            p = Stats + ns--;
        }
    }
    for (p = Stats + ns; p >= Stats; p--)
    {
        for (p1 = Suffix->Stats; p->Symbol != p1->Symbol; p1++)
            ;
        p->Flag = 0;
        p1->Flag = 1;
    }
    NumStats = ns;
    if (ns < 0)
        return NULL;
    SizeOfModel += 1 + 2 * (ns + 1);
    nc++;
    if (ns == 0)
    {
        tmp = Stats[0];
        oneState() = tmp;
    }
    return this;
}
void PPM_CONTEXT::write(int o, FILE* fp)
{
    STATE* p;
    int f, a, b, c;
    if ((int)nc < o)
        nc = o;
    putc(NumStats, fp);
    if (!NumStats)
    {
        f = (p = &oneState())->Freq;
        if (EscFreq)
            f = (2 * f) / EscFreq;
        f = CLAMP(f, 1, 127) | 0x80 * (p->Successor != NULL);
        putc(f, fp);
        putc(p->Symbol, fp);
        if (p->Successor)
            p->Successor->write(o + 1, fp);
        return;
    }
    for (p = Stats + 1; p <= Stats + NumStats; p++)
    {
        if (p[0].Freq > p[-1].Freq)
        {
            STATE* p1 = p;
            do
            {
                SWAP(p1[0], p1[-1]);
            } while (--p1 != Stats && p1[0].Freq > p1[-1].Freq);
        }
        if (p[0].Freq == p[-1].Freq && p[0].Successor && !p[-1].Successor)
        {
            STATE* p1 = p;
            do
            {
                SWAP(p1[0], p1[-1]);
            } while (--p1 != Stats && p1[0].Freq == p1[-1].Freq && !p1[-1].Successor);
        }
    }
    a = Stats->Freq + !Stats->Freq;
    f = (64 * EscFreq + (b = a >> 1)) / a;
    f = CLAMP(f, 1, 127) | 0x80 * (Stats->Successor != NULL);
    putc(f, fp);
    c = 64;
    for (p = Stats; p <= Stats + NumStats; p++)
    {
        f = (64 * p->Freq + b) / a;
        f += !f;
        if (p != Stats)
            putc((c - f) | 0x80 * (p->Successor != NULL), fp);
        c = f;
        putc(p->Symbol, fp);
    }
    for (p = Stats; p <= Stats + NumStats; p++)
        if (p->Successor)
            p->Successor->write(o + 1, fp);
}
void PPM_CONTEXT::write(int o, FILE* fp1, FILE* fp2, FILE* fp3)
{
    STATE *p, *p1;
    int f, a, b, c, d;
    if (!NumStats)
    {
        f = (p = &oneState())->Freq;
        if (EscFreq)
            f = (2 * f) / EscFreq;
        putc(QTable[CLAMP(f, 1, 196) - 1] | 0x80, fp1);
        if (Suffix->NumStats)
        {
            for (p1 = Suffix->Stats, d = 0; p1->Symbol != p->Symbol; p1++, d++)
                ;
            putc(d, fp2);
        }
        if (o < MaxOrder)
            putc(p->Successor != NULL, fp3);
        if (p->Successor)
            p->Successor->write(o + 1, fp1, fp2, fp3);
        return;
    }
    a = Stats->Freq + !Stats->Freq;
    f = (64 * EscFreq + (b = a >> 1)) / a;
    putc(QTable[CLAMP(f, 1, 196) - 1] | 0x80, fp1);
    memset(CharMask, 0, sizeof(CharMask));
    c = 64;
    for (p = Stats; p <= Stats + NumStats; p++)
    {
        f = (64 * p->Freq + b) / a;
        f += !f;
        if (p != Stats)
            putc(c - f, fp1);
        c = f;
        if (!Suffix)
            putc(p->Symbol, fp2);
        else if (p != Stats + NumStats || NumStats != Suffix->NumStats)
        {
            for (p1 = Suffix->Stats, d = 0; p1->Symbol != p->Symbol; p1++)
                d += !CharMask[p1->Symbol];
            putc(d, fp2);
            CharMask[p->Symbol] = 1;
        }
        if (o < MaxOrder)
            putc(p->Successor != NULL, fp3);
    }
    for (p = Stats; p <= Stats + NumStats; p++)
        if (p->Successor)
            p->Successor->write(o + 1, fp1, fp2, fp3);
}
void ShrinkModel(int MaxSize)
{
    memset(CharMask, 0, sizeof(CharMask));
    double s0 = 0.f, s1 = 0.f, b0 = 0.f, b1 = 0.f, b = 1.0;
    for (int k = 0;; k++)
    {
        printf("b: %6.2f, ", b);
        SizeOfModel = 1 + (nc = 1) + 2 * 256;
        for (int i = MaxOrder; i > 0; i--)
            MaxContext->cutOff(0, i, b);
        printf("SizeOfModel: %7lu bytes, nc: %7lu\n", SizeOfModel, nc);
        if (MaxSize >= (int)SizeOfModel)
            break;
        s0 = s1;
        b0 = b1;
        s1 = SizeOfModel;
        b1 = b;
        if (!k)
        {
            b++;
            continue;
        }
        if (s1 != s0)
            b = b0 + (b1 - b0) / (s1 - s0) * (MaxSize - s0);
        else
            b = b1 + 1.0;
        if (b > 2.0 * b1)
            b = 2.0 * b1;
        else if (b < 0.5 * b1)
            b = 0.5 * b1;
        if (s1 < 1.25 * MaxSize)
            b = 0.5 * b + 0.5 * b1;
        if (b - b1 < 0.01)
            b = b1 + 0.01;
    }
}
void _STDCALL EncodeFile(FILE* DecodedFile, int MaxOrder, int MaxSize)
{
    StartModelRare(MaxOrder);
    EncodeFile1(DecodedFile);
    ShrinkModel(MaxSize);
    FILE* fp = fopen("!PPMd.mdl", "wb");
    nc = 0;
    putc(MaxOrder, fp);
    MaxContext->write(0, fp);
    fseek(fp, 0, SEEK_SET);
    putc(nc, fp);
    fseek(fp, 0, SEEK_END);
    fclose(fp);
    //    FILE* fp1=fopen("!Freqs.mdl","wb"), * fp2=fopen("!PosSym.mdl","wb"), * fp3=fopen("!Flags.mdl","wb");
    //    putc(nc,fp1);                           MaxContext->write(0,fp1,fp2,fp3);
    //    fclose(fp1);        fclose(fp2);        fclose(fp3);
}
/*
#define MAKE_FOUR_CC(ch0,ch1,ch2,ch3)   \
((u32)(u8)(ch0)            |  \
((u32)(u8)(ch1) << 8)      |  \
((u32)(u8)(ch2) << 16)     |  \
((u32)(u8)(ch3) << 24 ))

void _STDCALL EncodeFileChunked(FILE* DecodedFile,int MaxOrder,int MaxSize)
{
    fseek(DecodedFile, 0, SEEK_END);

    long    src_sz      = ftell(DecodedFile);
    u8*     src_data	= new u8 [src_sz];

    fseek(DecodedFile, 0, SEEK_SET);
    fread( src_data, src_sz, 1, DecodedFile);

    u32 id = *((u32*)src_data);
    if(id != MAKE_FOUR_CC('B','I','N','S'))
    {
        printf("ERROR: bad bins file format");
        fclose(DecodedFile);
        delete[] src_data;
        return;
    }

    const u8*   data        = src_data + sizeof(u32);
    const u8*   data_end    = src_data + src_sz;
    unsigned    count       = 0;

    static char const * tmp_filename = "tmp_packet.bin";
    while( data < data_end )
    {
        FILE* tmp_file = fopen(tmp_filename, "wb");
        u16	sz	= *((u16*)data);
        data	+= sizeof(u16);
        fwrite(data, sz, 1, tmp_file);
        data	+= sz;
        fclose(tmp_file);
        FILE* tmp_r_file = fopen(tmp_filename, "rb");
        StartModelRare(MaxOrder);
        EncodeFile1(tmp_r_file);
        fclose(tmp_r_file);
    }
    fclose(DecodedFile);
    delete[] src_data;

    ShrinkModel(MaxSize);
    FILE* fp=fopen("!PPMd.mdl","wb");       nc=0;
    putc(MaxOrder,fp);                      MaxContext->write(0,fp);
    fseek(fp,0,SEEK_SET);                   putc(nc,fp);
    fseek(fp,0,SEEK_END);                   fclose(fp);
}
*/

extern int _PPM_MaxOrder;
extern int _PPM_SaSize;
extern int _PPM_ModelSize;

int MakePPMDictionaryFromFile(FILE* raw_bins_file_src)
{
    int MaxOrder = _PPM_MaxOrder;
    int SASize = _PPM_SaSize;
    int ModelSize = _PPM_ModelSize;

    StartSubAllocator(SASize);
    fseek(raw_bins_file_src, 0, SEEK_SET);
    EncodeFile(raw_bins_file_src, MaxOrder, ModelSize * 1024);
    return 0;
}

int MakePPMDictionary(char const* file_name)
{
    FILE* fpIn = fopen(file_name, "rb");
    if (!fpIn)
        return 1;
    MakePPMDictionaryFromFile(fpIn);
    fclose(fpIn);
    return 0;
}
