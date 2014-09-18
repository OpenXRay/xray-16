/****************************************************************************
 *  This file is part of PPMd project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 1997,        *
 *  1999-2001                                                               *
 *  Contents: PPMII model description and encoding/decoding routines        *
 ****************************************************************************/
#include <string.h>
#include "PPMd.h"
#pragma hdrstop
#include "Coder.hpp"
#include "SubAlloc.hpp"

using namespace ppmd;

#define CP printf("%s : %i\n",__FILE__,__LINE__ );


enum { UP_FREQ=5, INT_BITS=7, PERIOD_BITS=7, TOT_BITS=INT_BITS+PERIOD_BITS,
    INTERVAL=1 << INT_BITS, BIN_SCALE=1 << TOT_BITS, MAX_FREQ=124, O_BOUND=9 };

compression::ppmd::stream	*trained_model = 0;


template <class TMP_TYPE>
inline TMP_TYPE CLAMP(const TMP_TYPE& X,const TMP_TYPE& LoX,const TMP_TYPE& HiX) { return (X < LoX)?(LoX):((HiX < X)?(HiX):(X)); }


//==============================================================================
// SEE-contexts for PPM-contexts with masked symbols

#pragma pack(1)
struct SEE2_CONTEXT 
{ 
    WORD Summ;
    BYTE Shift, Count;
    void init(UINT InitVal) { Summ=InitVal << (Shift=PERIOD_BITS-4); Count=7; }
    UINT getMean() {
        UINT RetVal=(Summ >> Shift);        Summ -= RetVal;
        return RetVal+(RetVal == 0);
    }
    void update() {
        if (Shift < PERIOD_BITS && --Count == 0) {
            Summ += Summ;                   Count=3 << Shift++;
        }
    }
};
SEE2_CONTEXT _PACK_ATTR SEE2Cont[24][32], DummySEE2Cont;


//==============================================================================

struct 
PPM_CONTEXT 
{                 // Notes:
    BYTE NumStats, Flags;                   // 1. NumStats & NumMasked contain
    WORD SummFreq;                          //  number of symbols minus 1
    struct STATE {                          // 2. sizeof(WORD) > sizeof(BYTE)
        BYTE Symbol, Freq;                  // 3. contexts example:
        PPM_CONTEXT* Successor;             // MaxOrder:
    };

    STATE _PACK_ATTR*       Stats;          //  ABCD    context
    PPM_CONTEXT*            Suffix;         //   BCD    suffix
    
    inline void             encodeBinSymbol(int symbol);//   BCDE   successor
    inline void             encodeSymbol1(int symbol);// other orders:
    inline void             encodeSymbol2(int symbol);//   BCD    context
    inline void             decodeBinSymbol() const;//    CD    suffix
    inline void             decodeSymbol1();//   BCDE   successor
    inline void             decodeSymbol2();
    inline void             update1(STATE* p);
    inline void             update2(STATE* p);
    inline SEE2_CONTEXT*    makeEscFreq2() const;
    void                    rescale();
    void                    refresh(int OldNU,BOOL Scale);
    PPM_CONTEXT*            cutOff(int Order);
    PPM_CONTEXT*            removeBinConts(int Order);
    void                    makeSuffix();
    STATE&                  oneState() const { return (STATE&) SummFreq; }

    void                    read(_PPMD_FILE* fp,UINT PrevSym);
};
PPM_CONTEXT _PACK_ATTR* MaxContext;
#pragma pack()

static BYTE NS2BSIndx[256], QTable[260];    // constants
static PPM_CONTEXT::STATE* FoundState;      // found next state transition
static int  InitEsc, OrderFall, RunLength, InitRL, MaxOrder;
static BYTE CharMask[256], NumMasked, PrevSuccess, EscCount, PrintCount;
static WORD BinSumm[25][64];                // binary SEE-contexts
static MR_METHOD MRMethod;

static void _STDCALL StartModelRare(int MaxOrder,MR_METHOD MRMethod);

inline void SWAP(PPM_CONTEXT::STATE& s1,PPM_CONTEXT::STATE& s2)
{
    WORD t1=(WORD&) s1;                     PPM_CONTEXT* t2=s1.Successor;
    (WORD&) s1 = (WORD&) s2;                s1.Successor=s2.Successor;
    (WORD&) s2 = t1;                        s2.Successor=t2;
}
inline void StateCpy(PPM_CONTEXT::STATE& s1,const PPM_CONTEXT::STATE& s2)
{
    (WORD&) s1=(WORD&) s2;                  
    s1.Successor=s2.Successor;
}
struct PPMD_STARTUP { inline PPMD_STARTUP(); } PPMd_StartUp;
inline PPMD_STARTUP::PPMD_STARTUP()         // constants initialization
{
    UINT i, k, m, Step;
    for (i=0,k=1;i < N1     ;i++,k += 1)    Indx2Units[i]=k;
    for (k++;i < N1+N2      ;i++,k += 2)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3   ;i++,k += 3)    Indx2Units[i]=k;
    for (k++;i < N1+N2+N3+N4;i++,k += 4)    Indx2Units[i]=k;
    for (k=i=0;k < 128;k++) {
        i += (Indx2Units[i] < k+1);         Units2Indx[k]=i;
    }
    NS2BSIndx[0]=2*0;                       NS2BSIndx[1]=2*1;
    memset(NS2BSIndx+2,2*2,9);              memset(NS2BSIndx+11,2*3,256-11);
    for (i=0;i < UP_FREQ;i++)               QTable[i]=i;
    for (m=i=UP_FREQ, k=Step=1;i < 260;i++) {
        QTable[i]=m;
        if ( !--k ) { k = ++Step;           m++; }
    }
    (DWORD&) DummySEE2Cont=PPMdSignature;
}


void PPM_CONTEXT::makeSuffix()
{
    STATE* p, * p1;
    if ( !NumStats ) 
    {
        if ( !(p=&oneState())->Successor )  return;

        if ( !Suffix->NumStats )            
        {
            p1=&(Suffix->oneState());
        }
        else
        {
            for (p1=Suffix->Stats;p1->Symbol != p->Symbol;p1++)
                ;
        }
        
        p->Successor->Suffix = p1->Successor; 
        p->Successor->makeSuffix();
    } 
    else 
    {
        for (p=Stats;p <= Stats+NumStats;p++) 
        {
            if ( !p->Successor )            
                continue;

            if ( !Suffix )                  
            {
                p->Successor->Suffix=this;
            }
            else 
            {
                for (p1=Suffix->Stats;p1->Symbol != p->Symbol;p1++)
                    ;
                p->Successor->Suffix=p1->Successor;
            }
            p->Successor->makeSuffix();
        }
    }
}

void PPM_CONTEXT::read(_PPMD_FILE* fp,UINT PrevSym)
{
    STATE* p;                               Suffix=NULL;
    NumStats=_PPMD_E_GETC(fp);                     Flags=0x10*(PrevSym >= 0x40);
    if ( !NumStats ) {
        p=&oneState();                      p->Freq=_PPMD_E_GETC(fp);
        Flags |= 0x08*((p->Symbol=_PPMD_E_GETC(fp)) >= 0x40);
        if ((p->Freq & 0x80) != 0) {
            p->Freq &= ~0x80;
            p->Successor = (PPM_CONTEXT*) AllocContext();
            p->Successor->read(fp,p->Symbol);
        } else                              p->Successor=NULL;
        return;
    }
    Stats = (PPM_CONTEXT::STATE*) AllocUnits((NumStats+2) >> 1);
    for (p=Stats;p <= Stats+NumStats;p++) {
        p->Freq=_PPMD_E_GETC(fp);
        Flags |= 0x08*((p->Symbol=_PPMD_E_GETC(fp)) >= 0x40);
    }
    int EscFreq=SummFreq=(Stats->Freq & ~0x80);
    Flags |= 0x04*(EscFreq < NumStats && EscFreq < 127);
    for (p=Stats;p <= Stats+NumStats;p++) {
        if ((p->Freq & 0x80) != 0) {
            p->Freq &= ~0x80;
            p->Successor = (PPM_CONTEXT*) AllocContext();
            p->Successor->read(fp,p->Symbol);
        } else                              p->Successor=NULL;
        p->Freq=(p == Stats)?(64):(p[-1].Freq-p[0].Freq);
        SummFreq += p->Freq;
    }
    if (EscFreq > 32) {
        SummFreq=(EscFreq >> 1);
        for (p=Stats;p <= Stats+NumStats;p++)
                SummFreq += (p->Freq -= (3*p->Freq) >> 2);
    }
}
/*
void
PPM_CONTEXT::write( int o, FILE* fp )
{
    int EscFreq = EscCount; //(Stats)  ? Stats->Freq & ~0x80  : 0;
    int nc      = 0;

    STATE* p;
    int f, a, b, c;
    if (nc < o)                             
        nc=o;
        
    putc(NumStats,fp);
    if ( !NumStats ) {
        f=(p=&oneState())->Freq;
        if ( EscFreq )                      f=(2*f)/EscFreq;
        f=CLAMP(f,1,127) | 0x80*(p->Successor != NULL);
        putc(f,fp);                         putc(p->Symbol,fp);
        if ( p->Successor )                 p->Successor->write(o+1,fp);
        return;
    }
    for (p=Stats+1;p <= Stats+NumStats;p++) {
        if (p[0].Freq > p[-1].Freq) {
            STATE* p1=p;
            do { SWAP(p1[0],p1[-1]); } while (--p1 != Stats && p1[0].Freq > p1[-1].Freq);
        }
        if (p[0].Freq == p[-1].Freq && p[0].Successor && !p[-1].Successor) {
            STATE* p1=p;
            do { SWAP(p1[0],p1[-1]); } while (--p1 != Stats && p1[0].Freq == p1[-1].Freq && !p1[-1].Successor);
        }
    }
    a=Stats->Freq+!Stats->Freq;             f=(64*EscFreq+(b=a >> 1))/a;
    f=CLAMP(f,1,127) | 0x80*(Stats->Successor != NULL);
    putc(f,fp);                             c=64;
    for (p=Stats;p <= Stats+NumStats;p++) {
        f=(64*p->Freq+b)/a;                 f += !f;
        if (p != Stats)
            putc((c-f) | 0x80*(p->Successor != NULL),fp);
        c=f;                                putc(p->Symbol,fp);
    }
    for (p=Stats;p <= Stats+NumStats;p++)
            if ( p->Successor )             p->Successor->write(o+1,fp);
}
*/
void PPM_CONTEXT::refresh(int OldNU,BOOL Scale)
{
    int i=NumStats, EscFreq;
    STATE* p = Stats = (STATE*) ShrinkUnits(Stats,OldNU,(i+2) >> 1);
    Flags=(Flags & (0x10+0x04*Scale))+0x08*(p->Symbol >= 0x40);
    EscFreq=SummFreq-p->Freq;
    SummFreq = (p->Freq=(p->Freq+Scale) >> Scale);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Scale) >> Scale);
        Flags |= 0x08*(p->Symbol >= 0x40);
    } while ( --i );
    SummFreq += (EscFreq=(EscFreq+Scale) >> Scale);
}
#define P_CALL(F) ( PrefetchData(p->Successor), \
                    p->Successor=p->Successor->F(Order+1))
PPM_CONTEXT* PPM_CONTEXT::cutOff(int Order)
{
    int i, tmp;
    STATE* p;
    if ( !NumStats ) {
        if ((BYTE*) (p=&oneState())->Successor >= UnitsStart) {
            if (Order < MaxOrder)           P_CALL(cutOff);
            else                            p->Successor=NULL;
            if (!p->Successor && Order > O_BOUND)
                    goto REMOVE;
            return this;
        } else {
REMOVE:     SpecialFreeUnit(this);          return NULL;
        }
    }
    PrefetchData(Stats);
    Stats = (STATE*) MoveUnitsUp(Stats,tmp=(NumStats+2) >> 1);
    for (p=Stats+(i=NumStats);p >= Stats;p--)
            if ((BYTE*) p->Successor < UnitsStart) {
                p->Successor=NULL;          SWAP(*p,Stats[i--]);
            } else if (Order < MaxOrder)    P_CALL(cutOff);
            else                            p->Successor=NULL;
    if (i != NumStats && Order) {
        NumStats=i;                         p=Stats;
        if (i < 0) { FreeUnits(p,tmp);      goto REMOVE; }
        else if (i == 0) {
            Flags=(Flags & 0x10)+0x08*(p->Symbol >= 0x40);
            StateCpy(oneState(),*p);        FreeUnits(p,tmp);
            oneState().Freq=(oneState().Freq+11) >> 3;
        } else                              refresh(tmp,SummFreq > 16*i);
    }
    return this;
}
PPM_CONTEXT* PPM_CONTEXT::removeBinConts(int Order)
{
    STATE* p;
    if ( !NumStats ) {
        p=&oneState();
        if ((BYTE*) p->Successor >= UnitsStart && Order < MaxOrder)
                P_CALL(removeBinConts);
        else                                p->Successor=NULL;
        if (!p->Successor && (!Suffix->NumStats || Suffix->Flags == 0xFF)) {
            FreeUnits(this,1);              return NULL;
        } else                              return this;
    }
    PrefetchData(Stats);
    for (p=Stats+NumStats;p >= Stats;p--)
            if ((BYTE*) p->Successor >= UnitsStart && Order < MaxOrder)
                    P_CALL(removeBinConts);
            else                            p->Successor=NULL;
    return this;
}

static void RestoreModelRare(PPM_CONTEXT* pc1,PPM_CONTEXT* MinContext,
        PPM_CONTEXT* FSuccessor)
{
    PPM_CONTEXT* pc;
    PPM_CONTEXT::STATE* p;
    for (pc=MaxContext, pText=HeapStart;pc != pc1;pc=pc->Suffix)
            if (--(pc->NumStats) == 0) {
                pc->Flags=(pc->Flags & 0x10)+0x08*(pc->Stats->Symbol >= 0x40);
                p=pc->Stats;                StateCpy(pc->oneState(),*p);
                SpecialFreeUnit(p);
                pc->oneState().Freq=(pc->oneState().Freq+11) >> 3;
            } else
                    pc->refresh((pc->NumStats+3) >> 1,FALSE);
    for ( ;pc != MinContext;pc=pc->Suffix)
            if ( !pc->NumStats )
                    pc->oneState().Freq -= pc->oneState().Freq >> 1;
            else if ((pc->SummFreq += 4) > 128+4*pc->NumStats)
                    pc->refresh((pc->NumStats+2) >> 1,TRUE);
    if (MRMethod > MRM_FREEZE) {
        MaxContext=FSuccessor;              GlueCount += !(BList[1].Stamp & 1);
    } else if (MRMethod == MRM_FREEZE) {
        while ( MaxContext->Suffix )        MaxContext=MaxContext->Suffix;
        MaxContext->removeBinConts(0);      MRMethod=MR_METHOD(MRMethod+1);
        GlueCount=0;                        OrderFall=MaxOrder;
	} else if (MRMethod == MRM_RESTART || GetUsedMemory() < (SubAllocatorSize >> 1)) {
        StartModelRare(MaxOrder,MRMethod);
        EscCount=0;                         PrintCount=0xFF;
    } else {
        while ( MaxContext->Suffix )        MaxContext=MaxContext->Suffix;
        do {
            MaxContext->cutOff(0);          ExpandTextArea();
		} while (GetUsedMemory() > 3*(SubAllocatorSize >> 2));
        GlueCount=0;                        OrderFall=MaxOrder;
    }
}

static PPM_CONTEXT* _FASTCALL CreateSuccessors(BOOL Skip,PPM_CONTEXT::STATE* p,
        PPM_CONTEXT* pc);
static PPM_CONTEXT* _FASTCALL ReduceOrder(PPM_CONTEXT::STATE* p,PPM_CONTEXT* pc)
{
    PPM_CONTEXT::STATE* p1,  * ps[MAX_O], ** pps=ps;
    PPM_CONTEXT* pc1=pc, * UpBranch = (PPM_CONTEXT*) pText;
    BYTE tmp, sym=FoundState->Symbol;
    *pps++ = FoundState;                    FoundState->Successor=UpBranch;
    OrderFall++;
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    for ( ; ; ) {
        if ( !pc->Suffix ) {
            if (MRMethod > MRM_FREEZE) {
FROZEN:         do { (*--pps)->Successor = pc; } while (pps != ps);
                pText=HeapStart+1;          OrderFall=1;
            }
            return pc;
        }
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=2*(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else { p=&(pc->oneState());       p->Freq += (p->Freq < 32); }
LOOP_ENTRY:
        if ( p->Successor )                 break;
        *pps++ = p;                         p->Successor=UpBranch;
        OrderFall++;
    }
    if (MRMethod > MRM_FREEZE) {
        pc = p->Successor;                  goto FROZEN;
    } else if (p->Successor <= UpBranch) {
        p1=FoundState;                      FoundState=p;
        p->Successor=CreateSuccessors(FALSE,NULL,pc);
        FoundState=p1;
    }
    if (OrderFall == 1 && pc1 == MaxContext) {
        FoundState->Successor=p->Successor; pText--;
    }
    return p->Successor;
}
void PPM_CONTEXT::rescale()
{
    UINT OldNU, Adder, EscFreq, i=NumStats;
    STATE tmp, * p1, * p;
    for (p=FoundState;p != Stats;p--)       SWAP(p[0],p[-1]);
    p->Freq += 4;                           SummFreq += 4;
    EscFreq=SummFreq-p->Freq;
    Adder=(OrderFall != 0 || MRMethod > MRM_FREEZE);
    SummFreq = (p->Freq=(p->Freq+Adder) >> 1);
    do {
        EscFreq -= (++p)->Freq;
        SummFreq += (p->Freq=(p->Freq+Adder) >> 1);
        if (p[0].Freq > p[-1].Freq) {
            StateCpy(tmp,*(p1=p));
            do StateCpy(p1[0],p1[-1]); while (tmp.Freq > (--p1)[-1].Freq);
            StateCpy(*p1,tmp);
        }
    } while ( --i );
    if (p->Freq == 0) {
        do { i++; } while ((--p)->Freq == 0);
        EscFreq += i;                       OldNU=(NumStats+2) >> 1;
        if ((NumStats -= i) == 0) {
            StateCpy(tmp,*Stats);
            tmp.Freq=(2*tmp.Freq+EscFreq-1)/EscFreq;
            if (tmp.Freq > MAX_FREQ/3)      tmp.Freq=MAX_FREQ/3;
            FreeUnits(Stats,OldNU);         StateCpy(oneState(),tmp);
            Flags=(Flags & 0x10)+0x08*(tmp.Symbol >= 0x40);
            FoundState=&oneState();         return;
        }
        Stats = (STATE*) ShrinkUnits(Stats,OldNU,(NumStats+2) >> 1);
        Flags &= ~0x08;                     i=NumStats;
        Flags |= 0x08*((p=Stats)->Symbol >= 0x40);
        do { Flags |= 0x08*((++p)->Symbol >= 0x40); } while ( --i );
    }
    SummFreq += (EscFreq -= (EscFreq >> 1));
    Flags |= 0x04;                          FoundState=Stats;
}
static PPM_CONTEXT* _FASTCALL CreateSuccessors(BOOL Skip,PPM_CONTEXT::STATE* p,
        PPM_CONTEXT* pc)
{

    PPM_CONTEXT ct, * UpBranch=FoundState->Successor;
    PPM_CONTEXT::STATE* ps[MAX_O], ** pps=ps;
    UINT cf, s0;
    BYTE tmp, sym=FoundState->Symbol;
    if ( !Skip ) {
        *pps++ = FoundState;
        if ( !pc->Suffix )                  goto NO_LOOP;
    }
    if ( p ) { pc=pc->Suffix;               goto LOOP_ENTRY; }
    do {
        pc=pc->Suffix;
        if ( pc->NumStats ) {
            if ((p=pc->Stats)->Symbol != sym)
                    do { tmp=p[1].Symbol;   p++; } while (tmp != sym);
            tmp=(p->Freq < MAX_FREQ-9);
            p->Freq += tmp;                 pc->SummFreq += tmp;
        } else {
            p=&(pc->oneState());
            p->Freq += (!pc->Suffix->NumStats & (p->Freq < 24));
        }
LOOP_ENTRY:
        if (p->Successor != UpBranch) {
            pc=p->Successor;                break;
        }
        *pps++ = p;
    } while ( pc->Suffix );
NO_LOOP:
    if (pps == ps)                          return pc;
    ct.NumStats=0;                          ct.Flags=0x10*(sym >= 0x40);
    ct.oneState().Symbol=sym=*(BYTE*) UpBranch;
    ct.oneState().Successor=(PPM_CONTEXT*) (((BYTE*) UpBranch)+1);
    ct.Flags |= 0x08*(sym >= 0x40);
    if ( pc->NumStats ) {
        if ((p=pc->Stats)->Symbol != sym)
                do { tmp=p[1].Symbol;       p++; } while (tmp != sym);
        s0=pc->SummFreq-pc->NumStats-(cf=p->Freq-1);
        ct.oneState().Freq=1+((2*cf <= s0)?(5*cf > s0):((cf+2*s0-3)/s0));
    } else
            ct.oneState().Freq=pc->oneState().Freq;
    do {
        PPM_CONTEXT* pc1 = (PPM_CONTEXT*) AllocContext();
        if ( !pc1 )                         return NULL;
        ((DWORD*) pc1)[0] = ((DWORD*) &ct)[0];
        ((DWORD*) pc1)[1] = ((DWORD*) &ct)[1];
        pc1->Suffix=pc;                     (*--pps)->Successor=pc=pc1;
    } while (pps != ps);
    return pc;
}


static inline void UpdateModel( PPM_CONTEXT* MinContext)
{
    PPM_CONTEXT::STATE* p           = NULL;
    PPM_CONTEXT*        FSuccessor  = FoundState->Successor;
    PPM_CONTEXT*        pc          = MinContext->Suffix;
    PPM_CONTEXT*        pc1         = MaxContext;
    
    UINT ns1, ns, cf, sf, s0, FFreq=FoundState->Freq;
    BYTE Flag, sym, FSymbol=FoundState->Symbol;


    if( FFreq < MAX_FREQ/4  &&  pc ) 
    {
        if ( pc->NumStats ) 
        {
            if ((p=pc->Stats)->Symbol != FSymbol) 
            {
                do { sym=p[1].Symbol;       p++; } while (sym != FSymbol);
                if (p[0].Freq >= p[-1].Freq) 
                {
                    SWAP(p[0],p[-1]);       
                    p--;
                }
            }
            cf=2*(p->Freq < MAX_FREQ-9);
            p->Freq      += cf;                  
            pc->SummFreq += cf;
        } 
        else 
        { 
            p=&(pc->oneState());       
            p->Freq += (p->Freq < 32); 
        }
    }
    
    if( !OrderFall && FSuccessor) 
    {
        FoundState->Successor=CreateSuccessors(TRUE,p,MinContext);
        if ( !FoundState->Successor )       
            goto RESTART_MODEL;
        MaxContext=FoundState->Successor;   
        return;
    }

    *pText++ = FSymbol;                     

    PPM_CONTEXT*    Successor = (PPM_CONTEXT*) pText;
    
    if (pText >= UnitsStart)                
        goto RESTART_MODEL;


    if( FSuccessor ) 
    {
        if ((BYTE*) FSuccessor < UnitsStart)
            FSuccessor=CreateSuccessors(FALSE,p,MinContext);
    } 
    else
    {
        FSuccessor=ReduceOrder(p,MinContext);
    }
    
    if( !FSuccessor )                          
        goto RESTART_MODEL;


    if( !--OrderFall ) 
    {
        Successor=FSuccessor;               
        pText -= (MaxContext != MinContext);
    } 
    else if( MRMethod > MRM_FREEZE) 
    {
        Successor=FSuccessor;               
        pText=HeapStart;
        OrderFall=0;
    }
    
    s0=MinContext->SummFreq-(ns=MinContext->NumStats)-FFreq;
    for (Flag=0x08*(FSymbol >= 0x40);pc1 != MinContext;pc1=pc1->Suffix) 
    {
        if ((ns1=pc1->NumStats) != 0) 
        {
            if ((ns1 & 1) != 0) 
            {
                p=(PPM_CONTEXT::STATE*) ExpandUnits(pc1->Stats,(ns1+1) >> 1);
                if ( !p )                   
                    goto RESTART_MODEL;
                pc1->Stats=p;
            }
            pc1->SummFreq += (3*ns1+1 < ns);
        } 
        else 
        {
            p=(PPM_CONTEXT::STATE*) AllocUnits(1);
            if ( !p )                       
                goto RESTART_MODEL;
                
            StateCpy(*p,pc1->oneState());   
            pc1->Stats=p;
            
            if (p->Freq < MAX_FREQ/4-1)     p->Freq += p->Freq;
            else                            p->Freq  = MAX_FREQ-4;
            
            pc1->SummFreq=p->Freq+InitEsc+(ns > 2);
        }

        cf=2*FFreq*(pc1->SummFreq+6);       
        sf=s0+pc1->SummFreq;

        if (cf < 6*sf) 
        {
            cf=1+(cf > sf)+(cf >= 4*sf);
            pc1->SummFreq += 4;
        } 
        else 
        {
            cf=4+(cf > 9*sf)+(cf > 12*sf)+(cf > 15*sf);
            pc1->SummFreq += cf;
        }
        p=pc1->Stats+(++pc1->NumStats);     
        p->Successor=Successor;
        p->Symbol = FSymbol;                
        p->Freq = cf;
        pc1->Flags |= Flag;
    }

    MaxContext=FSuccessor;
    return;

RESTART_MODEL:
    RestoreModelRare(pc1,MinContext,FSuccessor);

}
// Tabulated escapes for exponential symbol distribution
static const BYTE ExpEscape[16]={ 25,14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2 };
#define GET_MEAN(SUMM,SHIFT,ROUND) ((SUMM+(1 << (SHIFT-ROUND))) >> (SHIFT))
inline void PPM_CONTEXT::encodeBinSymbol(int symbol)
{
    BYTE indx=NS2BSIndx[Suffix->NumStats]+PrevSuccess+Flags;
    STATE& rs=oneState();
    WORD& bs=BinSumm[QTable[rs.Freq-1]][indx+((RunLength >> 26) & 0x20)];
    UINT tmp=rcBinStart(bs,TOT_BITS);
    if (rs.Symbol == symbol) {
        FoundState=&rs;                     rs.Freq += (rs.Freq < 196);
        rcBinCorrect0(tmp);                 bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        PrevSuccess=1;                      RunLength++;
    } else {
        rcBinCorrect1(tmp,BIN_SCALE-bs);    bs -= GET_MEAN(bs,PERIOD_BITS,2);
        InitEsc=ExpEscape[bs >> 10];        CharMask[rs.Symbol]=EscCount;
        NumMasked=PrevSuccess=0;            FoundState=NULL;
    }
}
inline void PPM_CONTEXT::decodeBinSymbol() const
{
    BYTE indx=NS2BSIndx[Suffix->NumStats]+PrevSuccess+Flags;
    STATE& rs=oneState();

    WORD& bs=BinSumm[QTable[rs.Freq-1]][indx+((RunLength >> 26) & 0x20)];
    UINT tmp=rcBinStart(bs,TOT_BITS);
    if ( !rcBinDecode(tmp) ) {
        FoundState=&rs;                     rs.Freq += (rs.Freq < 196);
        rcBinCorrect0(tmp);                 bs += INTERVAL-GET_MEAN(bs,PERIOD_BITS,2);
        PrevSuccess=1;                      RunLength++;
    } else {
        rcBinCorrect1(tmp,BIN_SCALE-bs);    bs -= GET_MEAN(bs,PERIOD_BITS,2);
        InitEsc=ExpEscape[bs >> 10];        CharMask[rs.Symbol]=EscCount;
        NumMasked=PrevSuccess=0;            FoundState=NULL;
    }
}
inline void PPM_CONTEXT::update1(STATE* p) 
{
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p[0].Freq > p[-1].Freq) {
        SWAP(p[0],p[-1]);                   FoundState=--p;
        if (p->Freq > MAX_FREQ)             rescale();
    }
}
inline void PPM_CONTEXT::encodeSymbol1(int symbol)
{
    UINT LoCnt, i=Stats->Symbol;
    STATE* p=Stats;                         SubRange.scale=SummFreq;
    if (i == symbol) {
        PrevSuccess=(2*(SubRange.high=p->Freq) >= SubRange.scale);
        (FoundState=p)->Freq += 4;          SummFreq += 4;
        RunLength += PrevSuccess;
        if (p->Freq > MAX_FREQ)             rescale();
        SubRange.low=0;                        return;
    }
    LoCnt=p->Freq;
    i=NumStats;                             PrevSuccess=0;
    while ((++p)->Symbol != symbol) {
        LoCnt += p->Freq;
        if (--i == 0) {
            if ( Suffix )                   PrefetchData(Suffix);
            SubRange.low=LoCnt;                CharMask[p->Symbol]=EscCount;
            i=NumMasked=NumStats;           FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            SubRange.high=SubRange.scale;         return;
        }
    }
    SubRange.high=(SubRange.low=LoCnt)+p->Freq;   update1(p);
}
inline void PPM_CONTEXT::decodeSymbol1()
{
    UINT i, count, HiCnt=Stats->Freq;
    STATE* p=Stats;                         SubRange.scale=SummFreq;
    if ((count=rcGetCurrentCount()) < HiCnt) {
        PrevSuccess=(2*(SubRange.high=HiCnt) >= SubRange.scale);

        (FoundState=p)->Freq=(HiCnt += 4);  
        SummFreq += 4;

        RunLength += PrevSuccess;
        if (HiCnt > MAX_FREQ)               rescale();
        SubRange.low=0;                        return;
    }
    i=NumStats;                             PrevSuccess=0;
    while ((HiCnt += (++p)->Freq) <= count)
        if (--i == 0) {
            if ( Suffix )                   PrefetchData(Suffix);
            SubRange.low=HiCnt;                CharMask[p->Symbol]=EscCount;
            i=NumMasked=NumStats;           FoundState=NULL;
            do { CharMask[(--p)->Symbol]=EscCount; } while ( --i );
            SubRange.high=SubRange.scale;         return;
        }
    SubRange.low=(SubRange.high=HiCnt)-p->Freq;   
    ((PPM_CONTEXT*)this)->update1(p);
}
inline void PPM_CONTEXT::update2(STATE* p)
{
/*
    ++EscCount;
    RunLength = InitRL;
*/
    (FoundState=p)->Freq += 4;              SummFreq += 4;
    if (p->Freq > MAX_FREQ)                 rescale();
    EscCount++;                             RunLength=InitRL;
}
inline SEE2_CONTEXT* PPM_CONTEXT::makeEscFreq2() const
{
    BYTE* pb=(BYTE*) Stats;                 UINT t=2*NumStats;
    PrefetchData(pb);                       PrefetchData(pb+t);
    PrefetchData(pb += 2*t);                PrefetchData(pb+t);
    SEE2_CONTEXT* psee2c;
    if (NumStats != 0xFF) {
        t=Suffix->NumStats;
        psee2c=SEE2Cont[QTable[NumStats+2]-3]+(SummFreq > 11*(NumStats+1));
        psee2c += 2*(2*NumStats < t+NumMasked)+Flags;
        SubRange.scale=psee2c->getMean();
    } else {
        psee2c=&DummySEE2Cont;              SubRange.scale=1;
    }
    return psee2c;
}
inline void PPM_CONTEXT::encodeSymbol2(int symbol)
{
    SEE2_CONTEXT* psee2c=makeEscFreq2();
    UINT Sym, LoCnt=0, i=NumStats-NumMasked;
    STATE* p1, * p=Stats-1;
    do {
        do { Sym=p[1].Symbol;   p++; } while (CharMask[Sym] == EscCount);
        CharMask[Sym]=EscCount;
        if (Sym == symbol)                  goto SYMBOL_FOUND;
        LoCnt += p->Freq;
    } while ( --i );
    SubRange.high=(SubRange.scale += (SubRange.low=LoCnt));
    psee2c->Summ += SubRange.scale;            NumMasked = NumStats;
    return;
SYMBOL_FOUND:
    SubRange.low=LoCnt;                        SubRange.high=(LoCnt += p->Freq);
    for (p1=p; --i ; ) {
        do { Sym=p1[1].Symbol;  p1++; } while (CharMask[Sym] == EscCount);
        LoCnt += p1->Freq;
    }
    SubRange.scale += LoCnt;
    psee2c->update();                       update2(p);
}
inline void PPM_CONTEXT::decodeSymbol2()
{
    SEE2_CONTEXT* psee2c=makeEscFreq2();
    UINT Sym, count, HiCnt=0, i=NumStats-NumMasked;
    STATE* ps[256], ** pps=ps, * p=Stats-1;
    do {
        do { Sym=p[1].Symbol;   p++; } while (CharMask[Sym] == EscCount);
        HiCnt += p->Freq;                   *pps++ = p;
    } while ( --i );
    SubRange.scale += HiCnt;                   count=rcGetCurrentCount();
    p=*(pps=ps);
    if (count < HiCnt) {
        HiCnt=0;
        while ((HiCnt += p->Freq) <= count) p=*++pps;
        SubRange.low = (SubRange.high=HiCnt)-p->Freq;
        psee2c->update();                   
        update2(p);
    } else {
        SubRange.low=HiCnt;                    SubRange.high=SubRange.scale;
        i=NumStats-NumMasked;               NumMasked = NumStats;
        do { CharMask[(*pps)->Symbol]=EscCount; pps++; } while ( --i );
        psee2c->Summ += SubRange.scale;
    }
}
inline void ClearMask(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile)
{
    EscCount=1;                             memset(CharMask,0,sizeof(CharMask));
    if (++PrintCount == 0)                  PrintInfo(DecodedFile,EncodedFile);
}


void _STDCALL EncodeFile(_PPMD_FILE* EncodedFile,_PPMD_FILE* DecodedFile,
                            int MaxOrder,MR_METHOD MRMethod)
{
    rcInitEncoder();
    StartModelRare(MaxOrder,MRMethod);

    for (PPM_CONTEXT* MinContext; ; ) 
    {
        BYTE    ns  =(MinContext=MaxContext)->NumStats;
        int     c   = _PPMD_E_GETC(DecodedFile);

        if( ns ) 
        {
            MinContext->encodeSymbol1(c);   
            rcEncodeSymbol();
        } 
        else
        {
            MinContext->encodeBinSymbol(c);
        }
        
        while( !FoundState ) 
        {
            rcEncNormalize(EncodedFile);
            do
            {
                OrderFall++;                
                MinContext=MinContext->Suffix;

                if( !MinContext )          
                    goto STOP_ENCODING;
            } while (MinContext->NumStats == NumMasked);

            MinContext->encodeSymbol2(c);   
            rcEncodeSymbol();
        }
        
        if (!OrderFall && (BYTE*) FoundState->Successor >= UnitsStart)
        {
            PrefetchData(MaxContext=FoundState->Successor);
        }
        else 
        {
            UpdateModel(MinContext);        
            PrefetchData(MaxContext);

            if (EscCount == 0 )
                ClearMask(EncodedFile,DecodedFile);
        }
        rcEncNormalize(EncodedFile);
    } // for (PPM_CONTEXT* MinContext; ; )
    
STOP_ENCODING:
    rcFlushEncoder(EncodedFile);            
    PrintInfo(DecodedFile,EncodedFile);
}

void _STDCALL DecodeFile(_PPMD_FILE* DecodedFile,_PPMD_FILE* EncodedFile,
                            int MaxOrder,MR_METHOD MRMethod)
{
    rcInitDecoder(EncodedFile);
    StartModelRare(MaxOrder,MRMethod);
    
    PPM_CONTEXT* MinContext=MaxContext;
    for( BYTE ns=MinContext->NumStats; ; )
    {
        if( ns )
        {
            MinContext->decodeSymbol1();    
            rcRemoveSubrange();
        } 
        else                             
        {
            MinContext->decodeBinSymbol();
        }
        
        while( !FoundState ) 
        {
            rcDecNormalize(EncodedFile);
            do 
            {
                OrderFall++;                
                MinContext=MinContext->Suffix;
                if( !MinContext )          
                    goto STOP_DECODING;
            } 
            while( MinContext->NumStats == NumMasked );

            MinContext->decodeSymbol2();    
            rcRemoveSubrange();
        }
        _PPMD_D_PUTC(FoundState->Symbol,DecodedFile);
        if (!OrderFall && (BYTE*) FoundState->Successor >= UnitsStart)
        {
            PrefetchData(MaxContext=FoundState->Successor);
        }
        else 
        {
            UpdateModel(MinContext);
            PrefetchData(MaxContext);
            if (EscCount == 0)              
                ClearMask(EncodedFile,DecodedFile);
        }
        ns=(MinContext=MaxContext)->NumStats;
        rcDecNormalize(EncodedFile);
    }
    
STOP_DECODING:
    PrintInfo(DecodedFile,EncodedFile);
}

static void _STDCALL StartModelRare(int MaxOrder,MR_METHOD MRMethod)
{
    static bool         first_time  = true;
    static PPM_CONTEXT* context     = 0;

    if( first_time )
    {
        UINT i, k, m;

        memset( CharMask, 0, sizeof(CharMask) );
        EscCount=PrintCount=1;
        if( MaxOrder < 2 ) // we are in solid mode
        {                     
            OrderFall = ::MaxOrder;
            for( PPM_CONTEXT* pc=MaxContext; pc&&pc->Suffix; pc=pc->Suffix )
                OrderFall--;
            return;
        }

        ::OrderFall = ::MaxOrder = MaxOrder;          
        ::MRMethod  = MRMethod;
        
        InitSubAllocator();
        RunLength = InitRL = -((MaxOrder < 12) ? MaxOrder : 12) - 1;

        static const WORD InitBinEsc[] = {0x3CDD,0x1F3F,0x59BF,0x48F3,0x64A1,0x5ABC,0x6632,0x6051};

        for( i=m=0; m<25; m++ ) 
        {
            while( QTable[i] == m )              
                i++;
                
            for( k=0; k<8; k++ )
                BinSumm[m][k] = BIN_SCALE - InitBinEsc[k]/(i+1);
            for( k=8; k<64; k+=8 )
                memcpy( BinSumm[m]+k, BinSumm[m], 8*sizeof(WORD) );
        }
        for( i=m=0; m<24; m++ ) 
        {
            while( QTable[i+3] == m+3 )          
                i++;
            SEE2Cont[m][0].init(2*i+5);
            for( k=1; k<32; k++ )
                SEE2Cont[m][k] = SEE2Cont[m][0];
        }
        
        MaxContext = (PPM_CONTEXT*) AllocContext();
        MaxContext->Suffix = NULL;

        if( !trained_model || _PPMD_E_GETC(trained_model) > MaxOrder ) 
        {
            MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
            MaxContext->Stats = (PPM_CONTEXT::STATE*) AllocUnits(256/2);
            for (PrevSuccess=i=0;i < 256;i++) 
            {
                MaxContext->Stats[i].Symbol=i;  MaxContext->Stats[i].Freq=1;
                MaxContext->Stats[i].Successor=NULL;
            }
        } 
        else 
        {
            MaxContext->read(trained_model,0xFF);
            MaxContext->makeSuffix();
        }

//        first_time  = false;
        context     = MaxContext;
    }
    else
    {    
    
        UINT i, k, m;

        memset( CharMask, 0, sizeof(CharMask) );
        EscCount=PrintCount=1;
        ::OrderFall = ::MaxOrder = MaxOrder;          
        ::MRMethod  = MRMethod;
        
///        InitSubAllocator();
        RunLength = InitRL = -((MaxOrder < 12) ? MaxOrder : 12) - 1;
///        MaxContext = (PPM_CONTEXT*) AllocContext();
///        MaxContext->Suffix = NULL;

        MaxContext  = context;
        FoundState  = 0;

/*
        if( !trained_model || _PPMD_E_GETC(trained_model) > MaxOrder ) 
        {
            MaxContext->SummFreq=(MaxContext->NumStats=255)+2;
            MaxContext->Stats = (PPM_CONTEXT::STATE*) AllocUnits(256/2);
            for( PrevSuccess=i=0;i < 256;i++) 
            {
                MaxContext->Stats[i].Symbol=i;  MaxContext->Stats[i].Freq=1;
                MaxContext->Stats[i].Successor=NULL;
            }
        } 
        else 
        {
            MaxContext->read(trained_model,0xFF);
            MaxContext->makeSuffix();
        }
*/
    }
}

