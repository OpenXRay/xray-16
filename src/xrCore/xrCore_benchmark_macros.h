#pragma once

//#define BENCHMARK_BUILD

#ifdef BENCHMARK_BUILD
#define BENCH_SEC_CALLCONV __stdcall
#define BENCH_SEC_SCRAMBLEVTBL1 virtual int GetFlags() { return 1; }
#define BENCH_SEC_SCRAMBLEVTBL2 virtual void* GetData() { return 0; }
#define BENCH_SEC_SCRAMBLEVTBL3 virtual void* GetCache() { return 0; }
#define BENCH_SEC_SIGN , void* pBenchScrampleVoid = 0
#define BENCH_SEC_SCRAMBLEMEMBER1 float m_fSrambleMember1;
#define BENCH_SEC_SCRAMBLEMEMBER2 float m_fSrambleMember2;
#else // BENCHMARK_BUILD
#define BENCH_SEC_CALLCONV
#define BENCH_SEC_SCRAMBLEVTBL1
#define BENCH_SEC_SCRAMBLEVTBL2
#define BENCH_SEC_SCRAMBLEVTBL3
#define BENCH_SEC_SIGN
#define BENCH_SEC_SCRAMBLEMEMBER1
#define BENCH_SEC_SCRAMBLEMEMBER2
#endif // BENCHMARK_BUILD
