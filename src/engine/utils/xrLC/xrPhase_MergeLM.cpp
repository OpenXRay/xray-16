
#include "stdafx.h"
#include "build.h"

#include "xrPhase_MergeLM_Rect.h"
#include "../xrlc_light/xrdeflector.h"
#include "../xrlc_light/xrlc_globaldata.h"
#include "../xrlc_light/lightmap.h"
// Surface access
extern void _InitSurface	();
extern BOOL _rect_place		(L_rect &r, lm_layer*		D);

IC int	compare_defl		(CDeflector* D1, CDeflector* D2)
{
	// First  - by material
	u16 M1		= D1->GetBaseMaterial();
	u16 M2		= D2->GetBaseMaterial();
	if (M1<M2)	return	1;  // less
	if (M1>M2)	return	0;	// more
	return				2;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC int	sort_defl_analyze	(CDeflector* D1, CDeflector* D2)
{
	// first  - get material index
	u16 M1		= D1->GetBaseMaterial();
	u16 M2		= D2->GetBaseMaterial();

	// 1. material area
	u32	 A1		= pBuild->materials()[M1].internal_max_area;
	u32	 A2		= pBuild->materials()[M2].internal_max_area;
	if (A1<A2)	return	2;	// A2 better
	if (A1>A2)	return	1;	// A1 better

	// 2. material sector (geom - locality)
	u32	 s1		= pBuild->materials()[M1].sector;
	u32	 s2		= pBuild->materials()[M2].sector;
	if (s1<s2)	return	2;	// s2 better
	if (s1>s2)	return	1;	// s1 better

	// 3. just material index
	if (M1<M2)	return	2;	// s2 better
	if (M1>M2)	return	1;	// s1 better

	// 4. deflector area
	u32 da1		= D1->layer.Area();
	u32 da2		= D2->layer.Area();
	if (da1<da2)return	2;	// s2 better
	if (da1>da2)return	1;	// s1 better

	// 5. they are EQUAL
	return				0;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC bool	sort_defl_complex	(CDeflector* D1, CDeflector* D2)
{
	switch (sort_defl_analyze(D1,D2))	
	{
	case 1:		return true;	// 1st is better 
	case 2:		return false;	// 2nd is better
	case 0:		return false;	// none is better
	default:	return false;
	}
}

class	pred_remove { public: IC bool	operator() (CDeflector* D) { { if (0==D) return TRUE;}; if (D->bMerged) {D->bMerged=FALSE; return TRUE; } else return FALSE;  }; };

void CBuild::xrPhase_MergeLM()
{
	vecDefl			Layer;

	// **** Select all deflectors, which contain this light-layer
	Layer.clear	();
	for (u32 it=0; it<lc_global_data()->g_deflectors().size(); it++)
	{
		CDeflector*	D		= lc_global_data()->g_deflectors()[it];
		if (D->bMerged)		continue;
		Layer.push_back		(D);
	}

	// Merge this layer (which left unmerged)
	while (Layer.size()) 
	{
		VERIFY( lc_global_data() );
		string512	phase_name;
		xr_sprintf		(phase_name,"Building lightmap %d...", lc_global_data()->lightmaps().size());
		Phase		(phase_name);

		// Sort layer by similarity (state changes)
		// + calc material area
		Status		("Selection...");
		for (u32 it=0; it<materials().size(); it++) materials()[it].internal_max_area	= 0;
		for (u32 it=0; it<Layer.size(); it++)	{
			CDeflector*	D		= Layer[it];
			materials()[D->GetBaseMaterial()].internal_max_area	= _max(D->layer.Area(),materials()[D->GetBaseMaterial()].internal_max_area);
		}
		std::stable_sort(Layer.begin(),Layer.end(),sort_defl_complex);

		// Select first deflectors which can fit
		Status		("Selection...");
		u32 maxarea		= c_LMAP_size*c_LMAP_size*8;	// Max up to 8 lm selected
		u32 curarea		= 0;
		u32 merge_count	= 0;
		for (u32 it=0; it<(int)Layer.size(); it++)	{
			int		defl_area	= Layer[it]->layer.Area();
			if (curarea + defl_area > maxarea) break;
			curarea		+=	defl_area;
			merge_count ++;
		}

		// Startup
		Status		("Processing...");
		_InitSurface			();
		CLightmap*	lmap		= xr_new<CLightmap> ();
		VERIFY( lc_global_data() );
		lc_global_data()->lightmaps().push_back	(lmap);

		// Process 
		for (u32 it=0; it<merge_count; it++) 
		{
			if (0==(it%1024))	Status	("Process [%d/%d]...",it,merge_count);
			lm_layer&	L		= Layer[it]->layer;
			L_rect		rT,rS; 
			rS.a.set	(0,0);
			rS.b.set	(L.width+2*BORDER-1, L.height+2*BORDER-1);
			rS.iArea	= L.Area();
			rT			= rS;
			if (_rect_place(rT,&L)) 
			{
				BOOL		bRotated;
				if (rT.SizeX() == rS.SizeX()) {
					R_ASSERT(rT.SizeY() == rS.SizeY());
					bRotated = FALSE;
				} else {
					R_ASSERT(rT.SizeX() == rS.SizeY());
					R_ASSERT(rT.SizeY() == rS.SizeX());
					bRotated = TRUE;
				}
				lmap->Capture		(Layer[it],rT.a.x,rT.a.y,rT.SizeX(),rT.SizeY(),bRotated);
				Layer[it]->bMerged	= TRUE;
			}
			Progress(_sqrt(float(it)/float(merge_count)));
		}
		Progress	(1.f);

		// Remove merged lightmaps
		Status			("Cleanup...");
		vecDeflIt last	= std::remove_if	(Layer.begin(),Layer.end(),pred_remove());
		Layer.erase		(last,Layer.end());

		// Save
		Status			("Saving...");
		VERIFY			( pBuild );
		lmap->Save		(pBuild->path);
	}
	VERIFY( lc_global_data() );
	clMsg		( "%d lightmaps builded", lc_global_data()->lightmaps().size() );

	// Cleanup deflectors
	Progress	(1.f);
	Status		("Destroying deflectors...");
	for (u32 it=0; it<lc_global_data()->g_deflectors().size(); it++)
		xr_delete(lc_global_data()->g_deflectors()[it]);
	lc_global_data()->g_deflectors().clear_and_free	();
}

















/*

#include "stdafx.h"
#include "build.h"

#include "xrPhase_MergeLM_Rect.h"
#include "../xrlc_light/xrdeflector.h"
#include "../xrlc_light/xrlc_globaldata.h"
#include "../xrlc_light/lightmap.h"
// Surface access
extern void _InitSurface	();
extern BOOL _rect_place		(L_rect &r, lm_layer*		D);

IC int	compare_defl		(CDeflector* D1, CDeflector* D2)
{
	// First  - by material
	u16 M1		= D1->GetBaseMaterial();
	u16 M2		= D2->GetBaseMaterial();
	if (M1<M2)	return	1;  // less
	if (M1>M2)	return	0;	// more
	return				2;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC int	sort_defl_analyze	(CDeflector* D1, CDeflector* D2)
{
	// first  - get material index
	u16 M1		= D1->GetBaseMaterial();
	u16 M2		= D2->GetBaseMaterial();

	// 1. material area
	u32	 A1		= pBuild->materials()[M1].internal_max_area;
	u32	 A2		= pBuild->materials()[M2].internal_max_area;
	if (A1<A2)	return	2;	// A2 better
	if (A1>A2)	return	1;	// A1 better

	// 2. material sector (geom - locality)
	u32	 s1		= pBuild->materials()[M1].sector;
	u32	 s2		= pBuild->materials()[M2].sector;
	if (s1<s2)	return	2;	// s2 better
	if (s1>s2)	return	1;	// s1 better

	// 3. just material index
	if (M1<M2)	return	2;	// s2 better
	if (M1>M2)	return	1;	// s1 better

	// 4. deflector area
	u32 da1		= D1->layer.Area();
	u32 da2		= D2->layer.Area();
	if (da1<da2)return	2;	// s2 better
	if (da1>da2)return	1;	// s1 better

	// 5. they are EQUAL
	return				0;	// equal
}

// should define LESS(D1<D2) behaviour
// sorting - in increasing order
IC bool	sort_defl_complex	(CDeflector* D1, CDeflector* D2)
{
	switch (sort_defl_analyze(D1,D2))	
	{
	case 1:		return true;	// 1st is better 
	case 2:		return false;	// 2nd is better
	case 0:		return false;	// none is better
	default:	return false;
	}
}

class	pred_remove { public: IC bool	operator() (CDeflector* D) { { if (0==D) return TRUE;}; if (D->bMerged) {D->bMerged=FALSE; return TRUE; } else return FALSE;  }; };

void MergeLM_process( u32 m_from , u32 m_to , vecDefl* pLayer , CLightmap* lmap );

typedef struct MERGELM_PARAMS
{
	u32 m_from;
	u32 m_to;
	vecDefl* Layer;
	CLightmap* lmap;
	HANDLE hEvents[3];	// 0=start,1=terminate,2=ready
} * LP_MERGELM_PARAMS;

static CRITICAL_SECTION mergelm_cs;
static BOOL mergelm_threads_initialized = FALSE;
static u32 mergelm_threads_count = 0;
static LPHANDLE mergelm_threads_handles = NULL;
static LPHANDLE mergelm_ready_events = NULL;
static LP_MERGELM_PARAMS mergelm_params = NULL;

DWORD WINAPI MergeThreadProc( LPVOID lpParameter )
{
	LP_MERGELM_PARAMS pParams = ( LP_MERGELM_PARAMS ) lpParameter;

	while( TRUE ) {
		// Wait for "start" and "terminate" events
		switch ( WaitForMultipleObjects( 2 , pParams->hEvents , FALSE , INFINITE ) ) {
			case WAIT_OBJECT_0 + 0 :
				MergeLM_process( pParams->m_from , pParams->m_to , pParams->Layer , pParams->lmap );
				// Signal "ready" event
				SetEvent( pParams->hEvents[ 2 ] );
				break;
			case WAIT_OBJECT_0 + 1 :
				ExitThread( 0 );
				break;
			default :
				// Error ?
				ExitThread( 1 );
				break;
		} // switch
	} // while

	return 0;
}


void InitMergeThreads()
{
	if ( mergelm_threads_initialized )
		return;
	
	SYSTEM_INFO SystemInfo;
	GetSystemInfo( &SystemInfo );
	mergelm_threads_count = SystemInfo.dwNumberOfProcessors;

	mergelm_threads_handles = (LPHANDLE) xr_malloc( mergelm_threads_count * sizeof( HANDLE ) );
	mergelm_ready_events = (LPHANDLE) xr_malloc( mergelm_threads_count * sizeof( HANDLE ) );
	mergelm_params = (LP_MERGELM_PARAMS) xr_malloc( mergelm_threads_count * sizeof( MERGELM_PARAMS ) );

	InitializeCriticalSection( &mergelm_cs );

	for ( u32 i = 0 ; i < mergelm_threads_count ; i++ ) {

		ZeroMemory( &mergelm_params[ i ] , sizeof( MERGELM_PARAMS ) );

		// Creating start,terminate,ready events for each thread
		for( u32 x = 0 ; x < 3 ; x++ )
			mergelm_params[ i ].hEvents[ x ] = CreateEvent( NULL , FALSE , FALSE , NULL );

		// Duplicate ready event into array
		mergelm_ready_events[ i ] = mergelm_params[ i ].hEvents[ 2 ];

		mergelm_threads_handles[ i ] = CreateThread( NULL , 0 , &MergeThreadProc , &mergelm_params[ i ] , 0 , NULL );
	}

	mergelm_threads_initialized = TRUE;
}

void DoneMergeThreads()
{
	if ( ! mergelm_threads_initialized )
		return;

	// Asking helper threads to terminate
	for ( u32 i = 0 ; i < mergelm_threads_count ; i++ )
		SetEvent( mergelm_params[ i ].hEvents[ 1 ] );

	// Waiting threads for completion
	WaitForMultipleObjects( mergelm_threads_count , mergelm_threads_handles , TRUE , INFINITE );

	// Deleting events
	for ( u32 i = 0 ; i < mergelm_threads_count ; i++ )
		for( u32 x = 0 ; x < 3 ; x++ )
			CloseHandle( mergelm_params[ i ].hEvents[ x ] );

	// Freeing resources
	DeleteCriticalSection( &mergelm_cs );

	xr_free( mergelm_threads_handles );		mergelm_threads_handles = NULL;
	xr_free( mergelm_ready_events );		mergelm_ready_events = NULL;
	xr_free( mergelm_params );				mergelm_params = NULL;

	mergelm_threads_count = 0;

	mergelm_threads_initialized = FALSE;
}


void MergeLM_process_threads( u32 m_count , vecDefl* pLayer , CLightmap* lmap )
{
	u32 m_range = m_count / mergelm_threads_count;

	for ( u32 i = 0 ; i < mergelm_threads_count ; i++ ) {
		mergelm_params[ i ].Layer = pLayer;
		mergelm_params[ i ].lmap = lmap;
		
		mergelm_params[ i ].m_from = i * m_range;
		mergelm_params[ i ].m_to = ( i == ( mergelm_threads_count - 1 ) ) ? m_count : mergelm_params[ i ].m_from + m_range;

		SetEvent( mergelm_params[ i ].hEvents[ 0 ] );
	} // for

	WaitForMultipleObjects( mergelm_threads_count , mergelm_ready_events , TRUE , INFINITE );
}


void MergeLM_process( u32 m_from , u32 m_to , vecDefl* pLayer , CLightmap* lmap )
{
	for ( u32 it = m_from ; it < m_to ; it++ ) {
		lm_layer& L = (*pLayer)[ it ]->layer;
		L_rect rT , rS; 
		rS.a.set( 0 , 0 );
		rS.b.set( L.width + 2*BORDER - 1 , L.height + 2*BORDER - 1 );
		rS.iArea = L.Area();
		rT = rS;
		if ( _rect_place( rT , &L ) ) {
			BOOL		bRotated;
			if ( rT.SizeX() == rS.SizeX() ) {
				R_ASSERT(rT.SizeY() == rS.SizeY());
				bRotated = FALSE;
			} else {
				R_ASSERT(rT.SizeX() == rS.SizeY());
				R_ASSERT(rT.SizeY() == rS.SizeX());
				bRotated = TRUE;
			}

			EnterCriticalSection( &mergelm_cs );
				lmap->Capture( (*pLayer)[ it ] , rT.a.x , rT.a.y , rT.SizeX() , rT.SizeY() , bRotated );
			LeaveCriticalSection( &mergelm_cs );

			(*pLayer)[it]->bMerged	= TRUE;
		} // if
	} // for
}

void CBuild::xrPhase_MergeLM()
{
	vecDefl			Layer;

	// Initialize helper threads
	InitMergeThreads();

	// **** Select all deflectors, which contain this light-layer
	Layer.clear	();
	for (u32 it=0; it<lc_global_data()->g_deflectors().size(); it++)
	{
		CDeflector*	D		= lc_global_data()->g_deflectors()[it];
		if (D->bMerged)		continue;
		Layer.push_back		(D);
	}

	// Merge this layer (which left unmerged)
	while (Layer.size()) 
	{
		VERIFY( lc_global_data() );
		string512	phase_name;
		xr_sprintf		(phase_name,"Building lightmap %d...", lc_global_data()->lightmaps().size());
		Phase		(phase_name);

		// Sort layer by similarity (state changes)
		// + calc material area
		Status		("Selection...");
		for (u32 it=0; it<materials().size(); it++) materials()[it].internal_max_area	= 0;
		for (u32 it=0; it<Layer.size(); it++)	{
			CDeflector*	D		= Layer[it];
			materials()[D->GetBaseMaterial()].internal_max_area	= _max(D->layer.Area(),materials()[D->GetBaseMaterial()].internal_max_area);
		}
		std::stable_sort(Layer.begin(),Layer.end(),sort_defl_complex);

		// Select first deflectors which can fit
		Status		("Selection...");
		u32 maxarea		= c_LMAP_size*c_LMAP_size*8;	// Max up to 8 lm selected
		u32 curarea		= 0;
		u32 merge_count	= 0;
		for (u32 it=0; it<(int)Layer.size(); it++)	{
			int		defl_area	= Layer[it]->layer.Area();
			if (curarea + defl_area > maxarea) break;
			curarea		+=	defl_area;
			merge_count ++;
		}

		// Startup
		Status		("Processing...");
		_InitSurface			();
		CLightmap*	lmap		= xr_new<CLightmap> ();
		VERIFY( lc_global_data() );
		lc_global_data()->lightmaps().push_back	(lmap);

		// Process 
		MergeLM_process_threads( merge_count , &Layer , lmap );

		Progress	(1.f);

		// Remove merged lightmaps
		Status			("Cleanup...");
		vecDeflIt last	= std::remove_if	(Layer.begin(),Layer.end(),pred_remove());
		Layer.erase		(last,Layer.end());

		// Save
		Status			("Saving...");
		VERIFY			( pBuild );
		lmap->Save		(pBuild->path);
	}
	VERIFY( lc_global_data() );
	clMsg		( "%d lightmaps builded", lc_global_data()->lightmaps().size() );

	// Cleanup deflectors
	Progress	(1.f);
	Status		("Destroying deflectors...");
	for (u32 it=0; it<lc_global_data()->g_deflectors().size(); it++)
		xr_delete(lc_global_data()->g_deflectors()[it]);
	lc_global_data()->g_deflectors().clear_and_free	();

	// Destroy helper threads
	DoneMergeThreads();
}
*/
