#define _FASTCALL __fastcall
#define _STDCALL __stdcall
template <class TMP_TYPE>
inline TMP_TYPE CLAMP(const TMP_TYPE& X, const TMP_TYPE& LoX, const TMP_TYPE& HiX)
{
    return (X < LoX) ? (LoX) : ((HiX < X) ? (HiX) : (X));
}
template <class TMP_TYPE>
inline void SWAP(TMP_TYPE& t1, TMP_TYPE& t2)
{
    TMP_TYPE tmp = t1;
    t1 = t2;
    t2 = tmp;
}

const u32 N1 = 4, N2 = 4, N3 = 4, N4 = (128 + 3 - 1 * N1 - 2 * N2 - 3 * N3) / 4;
const u32 UNIT_SIZE = 20, N_INDEXES = N1 + N2 + N3 + N4;

#pragma pack(1)
struct MEM_BLK
{
    u32 Stamp, NU;
    MEM_BLK *next, *prev;
    u32 Dummy;
    void insertAt(MEM_BLK* p)
    {
        next = (prev = p)->next;
        p->next = next->prev = this;
    }
    void remove()
    {
        prev->next = next;
        next->prev = prev;
    }
} _PACK_ATTR;
#pragma pack()
static long SubAllocatorSize = 0;
static u8 Indx2Units[N_INDEXES], Units2Indx[128], GlueCount;
static u8 *HeapStart, *pText, *UnitsStart, *LoUnit, *HiUnit;
static struct NODE
{
    NODE* next;
} FreeList[N_INDEXES];

inline void InsertNode(void* p, int indx)
{
    ((NODE*)p)->next = FreeList[indx].next;
    FreeList[indx].next = (NODE*)p;
}
inline void* RemoveNode(int indx)
{
    NODE* RetVal = FreeList[indx].next;
    FreeList[indx].next = RetVal->next;
    return RetVal;
}
inline u32 U2B(int NU) { return 16 * NU + 4 * NU; }
inline void SplitBlock(void* pv, int OldIndx, int NewIndx)
{
    int i, UDiff = Indx2Units[OldIndx] - Indx2Units[NewIndx];
    u8* p = ((u8*)pv) + U2B(Indx2Units[NewIndx]);
    if (Indx2Units[i = Units2Indx[UDiff - 1]] != UDiff)
    {
        InsertNode(p, --i);
        p += U2B(i = Indx2Units[i]);
        UDiff -= i;
    }
    InsertNode(p, Units2Indx[UDiff - 1]);
}
u32 _STDCALL GetUsedMemory()
{
    u32 i, k, RetVal = SubAllocatorSize - (HiUnit - LoUnit) - (UnitsStart - pText);
    for (k = i = 0; i < N_INDEXES; i++, k = 0)
    {
        for (NODE* pn = FreeList + i; (pn = pn->next) != NULL; k++)
            ;
        RetVal -= UNIT_SIZE * Indx2Units[i] * k;
    }
    return (RetVal >> 2);
}
BOOL _STDCALL StartSubAllocator(int SASize)
{
    u32 t = SASize << 20;
    if ((HeapStart = new u8[t]) == NULL)
        return FALSE;
    SubAllocatorSize = t;
    return TRUE;
}
static inline void InitSubAllocator()
{
    int i, k;
    memset(FreeList, 0, sizeof(FreeList));
    HiUnit = (pText = HeapStart) + SubAllocatorSize;
    u32 Diff = UNIT_SIZE * (SubAllocatorSize / 8 / UNIT_SIZE * 7);
    LoUnit = UnitsStart = HiUnit - Diff;
    for (i = 0, k = 1; i < N1; i++, k += 1)
        Indx2Units[i] = k;
    for (k++; i < N1 + N2; i++, k += 2)
        Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3; i++, k += 3)
        Indx2Units[i] = k;
    for (k++; i < N1 + N2 + N3 + N4; i++, k += 4)
        Indx2Units[i] = k;
    for (GlueCount = k = i = 0; k < 128; k++)
    {
        i += (Indx2Units[i] < k + 1);
        Units2Indx[k] = i;
    }
}
static inline void GlueFreeBlocks()
{
    MEM_BLK s0, *p, *p1;
    int i, k, sz;
    if (LoUnit != HiUnit)
        *LoUnit = 0;
    for (i = 0, s0.next = s0.prev = &s0; i < N_INDEXES; i++)
        while (FreeList[i].next)
        {
            p = (MEM_BLK*)RemoveNode(i);
            p->insertAt(&s0);
            p->Stamp = ~0;
            p->NU = Indx2Units[i];
        }
    for (p = s0.next; p != &s0; p = p->next)
        while ((p1 = p + p->NU)->Stamp == u32(~0) && p->NU + p1->NU < 0x10000)
        {
            p1->remove();
            p->NU += p1->NU;
        }
    while ((p = s0.next) != &s0)
    {
        for (p->remove(), sz = p->NU; sz > 128; sz -= 128, p += 128)
            InsertNode(p, N_INDEXES - 1);
        if (Indx2Units[i = Units2Indx[sz - 1]] != sz)
        {
            k = sz - Indx2Units[--i];
            InsertNode(p + (sz - k), k - 1);
        }
        InsertNode(p, i);
    }
}
static void* AllocUnitsRare(int indx)
{
    if (!GlueCount)
    {
        GlueCount = 255;
        GlueFreeBlocks();
        if (FreeList[indx].next)
            return RemoveNode(indx);
    }
    int i = indx;
    do
    {
        if (++i == N_INDEXES)
        {
            GlueCount--;
            i = U2B(Indx2Units[indx]);
            return (UnitsStart - pText > i) ? (UnitsStart -= i) : (NULL);
        }
    } while (!FreeList[i].next);
    void* RetVal = RemoveNode(i);
    SplitBlock(RetVal, i, indx);
    return RetVal;
}
inline void* AllocUnits(int NU)
{
    int indx = Units2Indx[NU - 1];
    if (FreeList[indx].next)
        return RemoveNode(indx);
    void* RetVal = LoUnit;
    LoUnit += U2B(Indx2Units[indx]);
    if (LoUnit <= HiUnit)
        return RetVal;
    LoUnit -= U2B(Indx2Units[indx]);
    return AllocUnitsRare(indx);
}
inline void* AllocContext()
{
    if (HiUnit != LoUnit)
        return (HiUnit -= UNIT_SIZE);
    if (FreeList->next)
        return RemoveNode(0);
    return AllocUnitsRare(0);
}
inline void* ExpandUnits(void* OldPtr, int OldNU)
{
    int i0 = Units2Indx[OldNU - 1], i1 = Units2Indx[OldNU - 1 + 1];
    if (i0 == i1)
        return OldPtr;
    void* ptr = AllocUnits(OldNU + 1);
    if (ptr)
    {
        memcpy(ptr, OldPtr, U2B(OldNU));
        InsertNode(OldPtr, i0);
    }
    return ptr;
}
void _STDCALL PrintInfo(FILE* DecodedFile)
{
    u32 NDec = ftell(DecodedFile);
    u32 UsedMemory = GetUsedMemory();
    u32 m1 = UsedMemory >> 18;
    u32 m2 = (10U * (UsedMemory - (m1 << 18)) + (1 << 17)) >> 18;
    if (m2 == 10)
    {
        m1++;
        m2 = 0;
    }
    printf("%8d, used memory:%3d.%1dMB\r", NDec, m1, m2);
}
