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

struct {
	volatile LONG size;
	DWORD dwPadding[15];
} ttapi_queue_size;

DWORD WINAPI ttapiThreadProc( LPVOID lpParameter )
{
	LPTTAPI_WORKER_PARAMS pParams = ( LPTTAPI_WORKER_PARAMS ) lpParameter;
	DWORD i;

	while( TRUE ) {		
		// Wait

		// Fast
		for ( i = 0 ; i < 10000 ; ++i ) {
			if ( ! _InterlockedCompareExchange( &pParams->vlFlag , 0 , 0 ) )
				goto process;
			__asm pause;
		}
		// Moderate
		for ( i = 0 ; i < 1000000 ; ++i ) {
			if ( ! _InterlockedCompareExchange( &pParams->vlFlag , 0 , 0 ) )
				goto process;
			SwitchToThread();
		}
		// Slow
		for ( i = 0 ; i < 1000 ; ++i ) {
			if ( ! _InterlockedCompareExchange( &pParams->vlFlag , 0 , 0 ) )
				goto process;
			Sleep( 1 );
		}
		// VerySlow
		while ( _InterlockedCompareExchange( &pParams->vlFlag , 0 , 0 ) )
			Sleep( 100 );

		process:

		if ( pParams->lpWorkerFunc )
			pParams->lpWorkerFunc( pParams->lpvWorkerFuncParams );
		else
			break;

		_InterlockedExchange( &pParams->vlFlag , 1 );
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

DWORD ttapi_Init()
{
	if ( ttapi_initialized )
		return ttapi_workers_count;

	// System Info
	SYSTEM_INFO SystemInfo;
	GetSystemInfo( &SystemInfo );
	ttapi_workers_count = SystemInfo.dwNumberOfProcessors;
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

	SetThreadIdealProcessor( GetCurrentThread() , 0 );

	// Creating threads
	for ( DWORD i = 0 ; i < ttapi_threads_count ; i++ ) {

		// Initializing "enter" "critical section"
		_InterlockedExchange( &ttapi_worker_params[ i ].vlFlag , 1 );

		if ( ( ttapi_threads_handles[ i ] = CreateThread( NULL , 0 , &ttapiThreadProc , &ttapi_worker_params[ i ] , 0 , &dwThreadId ) ) == NULL )
			return 0;

		// Setting preferred processor
		SetThreadIdealProcessor( ttapi_threads_handles[ i ] , i + 1 );

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

	if ( ttapi_thread_workers ) {
		// Setting queue size
		_InterlockedExchange( &ttapi_queue_size.size , ttapi_thread_workers );

		// Starting all workers except the last
		for ( DWORD i = 0 ; i < ttapi_thread_workers ; ++i )
			_InterlockedExchange( &ttapi_worker_params[ i ].vlFlag , 0 );

		// Running last worker in current thread
		ttapi_worker_params[ ttapi_thread_workers ].lpWorkerFunc( ttapi_worker_params[ ttapi_thread_workers ].lpvWorkerFuncParams );

		// Waiting task queue to become empty
		while( _InterlockedCompareExchange( &ttapi_queue_size.size , 0 , 0 ) )
			__asm pause;
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
