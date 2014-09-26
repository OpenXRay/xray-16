#include "stdafx.h"
#pragma hdrstop

typedef struct TTAPI_WORKER_PARAMS {
	volatile LONG			vlFlag;
	LPPTTAPI_WORKER_FUNC	lpWorkerFunc;
	LPVOID					lpvWorkerFuncParams;
	DWORD					dwPadding[13];
} * PTTAPI_WORKER_PARAMS;

typedef PTTAPI_WORKER_PARAMS LPTTAPI_WORKER_PARAMS;

static LPHANDLE ttapi_threads_handles = NULL;
static BOOL ttapi_initialized = FALSE;
static DWORD ttapi_workers_count = 0;
static DWORD ttapi_threads_count = 0;
static DWORD ttapi_assigned_workers = 0;
static LPTTAPI_WORKER_PARAMS ttapi_worker_params = NULL;

static DWORD ttapi_dwFastIter = 0;
static DWORD ttapi_dwSlowIter = 0;

struct {
	volatile LONG size;
	DWORD dwPadding[15];
} ttapi_queue_size;

DWORD WINAPI ttapiThreadProc( LPVOID lpParameter )
{
	LPTTAPI_WORKER_PARAMS pParams = ( LPTTAPI_WORKER_PARAMS ) lpParameter;
	DWORD i , dwFastIter = ttapi_dwFastIter , dwSlowIter = ttapi_dwSlowIter;

	while( TRUE ) {		
		// Wait

		// Fast
		for ( i = 0 ; i < dwFastIter ; ++i ) {
			if ( pParams->vlFlag == 0 ) {
				// Msg( "0x%8.8X Fast %u" , dwId , i );
				goto process;
			}
			__asm pause;
		}

		// Moderate
		for ( i = 0 ; i < dwSlowIter ; ++i ) {
			if ( pParams->vlFlag == 0 ) {
				// Msg( "0x%8.8X Moderate %u" , dwId , i );
				goto process;
			}
			SwitchToThread();
		}

		// Slow
		while ( pParams->vlFlag ) {
			Sleep( 100 );
			//Msg( "Shit" );
		}

		process:

		pParams->vlFlag = 1;

		if ( pParams->lpWorkerFunc )
			pParams->lpWorkerFunc( pParams->lpvWorkerFuncParams );
		else
			break;

		_InterlockedDecrement( &ttapi_queue_size.size );

	} // while

	return 0;
}

typedef struct tagTHREADNAME_INFO {
  DWORD dwType;
  LPCSTR szName;
  DWORD dwThreadID;
  DWORD dwFlags;
} THREADNAME_INFO;

void SetThreadName( DWORD dwThreadID , LPCSTR szThreadName )
{
  THREADNAME_INFO info;
  {
    info.dwType = 0x1000;
    info.szName = szThreadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
  }
  __try
  {
    RaiseException( 0x406D1388, 0, sizeof(info)/sizeof(DWORD), (DWORD*)&info );
  }
  __except (EXCEPTION_CONTINUE_EXECUTION)
  {
  }
}

DWORD ttapi_Init( _processor_info* ID )
{
	if ( ttapi_initialized )
		return ttapi_workers_count;

	// System Info
	ttapi_workers_count = ID->n_cores;

	SetPriorityClass( GetCurrentProcess() , REALTIME_PRIORITY_CLASS );

	DWORD i , dwNumIter;
	volatile DWORD dwDummy = 1;
	LARGE_INTEGER liFrequency , liStart , liEnd;

	QueryPerformanceFrequency( &liFrequency );

	// Get fast spin-loop timings
	dwNumIter = 100000000;

	QueryPerformanceCounter( &liStart );
	for ( i = 0 ; i < dwNumIter ; ++i ) {
		if ( dwDummy == 0 )
			goto process1;
		__asm pause;
	}
	process1:
	QueryPerformanceCounter( &liEnd );

	// We want 1/25 (40ms) fast spin-loop
	ttapi_dwFastIter = ( dwNumIter * liFrequency.QuadPart ) / ( ( liEnd.QuadPart - liStart.QuadPart ) * 25 );
	//Msg( "fast spin-loop iterations : %u" , ttapi_dwFastIter );

	// Get slow spin-loop timings
	dwNumIter = 10000000;

	QueryPerformanceCounter( &liStart );
	for ( i = 0 ; i < dwNumIter ; ++i ) {
		if ( dwDummy == 0 )
			goto process2;
		SwitchToThread();
	}
	process2:
	QueryPerformanceCounter( &liEnd );

	// We want 1/2 (500ms) slow spin-loop
	ttapi_dwSlowIter = ( dwNumIter * liFrequency.QuadPart ) / ( ( liEnd.QuadPart - liStart.QuadPart ) * 2 );
	//Msg( "slow spin-loop iterations : %u" , ttapi_dwSlowIter );

	SetPriorityClass( GetCurrentProcess() , NORMAL_PRIORITY_CLASS );

	// Check for override from command line
	char szSearchFor[] = "-max-threads";
	char* pszTemp = strstr( GetCommandLine() , szSearchFor );
	DWORD dwOverride = 0;
	if ( pszTemp )
		if ( sscanf_s( pszTemp + strlen( szSearchFor ) , "%u" , &dwOverride ) )
			if ( ( dwOverride >= 1 ) && ( dwOverride < ttapi_workers_count ) )
				ttapi_workers_count = dwOverride;

	// Number of helper threads
	ttapi_threads_count = ttapi_workers_count - 1;

	// Creating control structures
	if ( ( ttapi_threads_handles = (LPHANDLE) malloc( sizeof(HANDLE)*ttapi_threads_count ) ) == NULL )
		return 0;
	if ( ( ttapi_worker_params = (PTTAPI_WORKER_PARAMS) malloc( sizeof(TTAPI_WORKER_PARAMS)*ttapi_workers_count ) ) == NULL )
		return 0;

	// Clearing params
	for ( DWORD i = 0 ; i < ttapi_workers_count ; i++ )
		memset( &ttapi_worker_params[ i ] , 0 , sizeof( TTAPI_WORKER_PARAMS ) );

	char szThreadName[64];
	DWORD dwThreadId = 0;
	DWORD dwAffinitiMask = ID->affinity_mask;
	DWORD dwCurrentMask = 0x01;

	// Setting affinity
	while ( ! ( dwAffinitiMask & dwCurrentMask ) )
		dwCurrentMask <<= 1;

	SetThreadAffinityMask( GetCurrentThread() , dwCurrentMask );
	//Msg("Master Thread Affinity Mask : 0x%8.8X" , dwCurrentMask );

	// Creating threads
	for ( DWORD i = 0 ; i < ttapi_threads_count ; i++ ) {

		// Initializing "enter" "critical section"
		ttapi_worker_params[ i ].vlFlag = 1;

		if ( ( ttapi_threads_handles[ i ] = CreateThread( NULL , 0 , &ttapiThreadProc , &ttapi_worker_params[ i ] , 0 , &dwThreadId ) ) == NULL )
			return 0;

		// Setting affinity
		do
			dwCurrentMask <<= 1;
		while ( ! ( dwAffinitiMask & dwCurrentMask ) );
			
		SetThreadAffinityMask( ttapi_threads_handles[ i ] , dwCurrentMask );
		//Msg("Helper Thread #%u Affinity Mask : 0x%8.8X" , i + 1 , dwCurrentMask );

		// Setting thread name
		sprintf_s( szThreadName , "Helper Thread #%u" , i );
		SetThreadName( dwThreadId , szThreadName );
	}

	ttapi_initialized = TRUE;

	return ttapi_workers_count;
}

DWORD ttapi_GetWorkersCount()
{
	return ttapi_workers_count;
}

// We do not check for overflow here to be faster
// Assume that caller is smart enough to use ttapi_GetWorkersCount() to get number of available slots
VOID ttapi_AddWorker( LPPTTAPI_WORKER_FUNC lpWorkerFunc , LPVOID lpvWorkerFuncParams )
{	
	// Assigning parameters
	ttapi_worker_params[ ttapi_assigned_workers ].lpWorkerFunc = lpWorkerFunc;
	ttapi_worker_params[ ttapi_assigned_workers ].lpvWorkerFuncParams = lpvWorkerFuncParams;

	ttapi_assigned_workers++;	
}

VOID ttapi_RunAllWorkers()
{	
	DWORD ttapi_thread_workers = ( ttapi_assigned_workers - 1 );
	//unsigned __int64 Start,Stop;

	if ( ttapi_thread_workers ) {
		// Setting queue size
		ttapi_queue_size.size = ttapi_thread_workers;

		// Starting all workers except the last
		for ( DWORD i = 0 ; i < ttapi_thread_workers ; ++i )
			_InterlockedExchange( &ttapi_worker_params[ i ].vlFlag , 0 );

		// Running last worker in current thread
		ttapi_worker_params[ ttapi_thread_workers ].lpWorkerFunc( ttapi_worker_params[ ttapi_thread_workers ].lpvWorkerFuncParams );

		// Waiting task queue to become empty
		//Start = __rdtsc();
		while( ttapi_queue_size.size )
			__asm pause;
		//Stop = __rdtsc();
		//Msg( "Wait: %u ticks" , Stop - Start );


	} else
		// Running the only worker in current thread
		ttapi_worker_params[ ttapi_thread_workers ].lpWorkerFunc( ttapi_worker_params[ ttapi_thread_workers ].lpvWorkerFuncParams );

	// Cleaning active workers count
	ttapi_assigned_workers = 0;
}

VOID ttapi_Done()
{
	if ( ! ttapi_initialized )
		return;

	// Asking helper threads to terminate
	for ( DWORD i = 0 ; i < ttapi_threads_count ; i++ ) {
		ttapi_worker_params[ i ].lpWorkerFunc = NULL;
		_InterlockedExchange( &ttapi_worker_params[ i ].vlFlag , 0 );
	}

	// Waiting threads for completion
	WaitForMultipleObjects( ttapi_threads_count , ttapi_threads_handles , TRUE , INFINITE );

	// Freeing resources
	free( ttapi_threads_handles );		ttapi_threads_handles = NULL;
	free( ttapi_worker_params );		ttapi_worker_params = NULL;

	ttapi_workers_count = 0;
	ttapi_threads_count = 0;
	ttapi_assigned_workers = 0;

	ttapi_initialized = FALSE;
}
