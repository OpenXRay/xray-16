#include "stdafx.h"
#include "PropSlimTools.h"
#include "object.h"
#include "object_sliding.h"

#pragma warning(disable:4018)

static Object*					g_pObject						= 0;
static ArbitraryList<MeshPt*>	g_ppTempPts						= 0;
static float					g_fSlidingWindowErrorTolerance	= 0.1f;
static BOOL						g_bOptimiseVertexOrder			= FALSE;
static u32						g_bMaxSlidingWindow				= u32(-1);
static VIPM_Result*				g_pResult						= 0;

ETOOLS_API void			 __stdcall VIPM_Init			()
{
//.	OutputDebugString("VIPM_INIT-------------------\n");
	R_ASSERT2	(0==g_pObject,"VIPM already in use!");
	g_pObject							= xr_new<Object>();
	g_pResult							= xr_new<VIPM_Result>();
	g_pObject->iNumCollapses			= 0;
	g_pObject->iCurSlidingWindowLevel	= 0;
}

ETOOLS_API void			 __stdcall VIPM_AppendVertex	(const Fvector3& p, const Fvector2& uv)
{
	MeshPt* pt			= xr_new<MeshPt>	(&g_pObject->CurPtRoot);
	g_ppTempPts.push_back(pt);
	pt->mypt.vPos 		= p;
	pt->mypt.fU			= uv.x;
	pt->mypt.fV			= uv.y;
	pt->mypt.dwIndex	= g_ppTempPts.size()-1;
}

ETOOLS_API void			 __stdcall VIPM_AppendFace		(u16 v0, u16 v1, u16 v2)
{
	xr_new<MeshTri>(g_ppTempPts[v0],g_ppTempPts[v1],g_ppTempPts[v2], &g_pObject->CurTriRoot, &g_pObject->CurEdgeRoot );
}

void CalculateAllCollapses(Object* m_pObject, u32 max_sliding_window=u32(-1), float m_fSlidingWindowErrorTolerance=1.f)
{
	m_pObject->BinEdgeCollapse();
	while (true){
		// Find the best collapse you can.
		// (how expensive is this? Ohhhh yes).
		float		fBestError			= 1.0e10f;
		MeshEdge	*pedgeBestError		= NULL;
		MeshPt		*pptBestError		= NULL;
		// NL = NewLevel - would force a new level.
		float		fBestErrorNL		= 1.0e10f;
		MeshEdge	*pedgeBestErrorNL	= NULL;
		MeshPt		*pptBestErrorNL		= NULL;
		MeshPt		*ppt;
		MeshEdge	*pedge;

		float		fAverage			= 0.0f;
		int			iAvCount			= 0;

		// Flush the cache, just in case.
		m_pObject->FindCollapseError		( NULL, NULL, FALSE );

		for ( ppt = m_pObject->CurPtRoot.ListNext(); ppt != NULL; ppt = ppt->ListNext() ){
			if (0==ppt->FirstEdge())	continue;
			// Disallow any pts that are on an edge - shouldn't be collapsing them.
			BOOL bAllowed = TRUE;
			for ( pedge = ppt->FirstEdge(); pedge != NULL; pedge = ppt->NextEdge() ){
				if ( ( pedge->pTri12 == NULL ) || ( pedge->pTri21 == NULL ) ){
					// This edge does not have two tris on it - disallow it.
					bAllowed = FALSE;
					break;
				}
			}
			if ( !bAllowed ) continue;

			BOOL bRequiresNewLevel = FALSE;
			if ( !m_pObject->CollapseAllowedForLevel ( ppt, m_pObject->iCurSlidingWindowLevel ) ){
				// This collapse would force a new level.
				bRequiresNewLevel = TRUE;
			}

			// collect error
			for ( pedge = ppt->FirstEdge(); pedge != NULL; pedge = ppt->NextEdge() ){
				float fErrorBin = m_pObject->FindCollapseError ( ppt, pedge, TRUE );
				iAvCount++;
				fAverage += fErrorBin;
				if ( bRequiresNewLevel ){
					if ( fBestErrorNL > fErrorBin ){
						fBestErrorNL = fErrorBin;
						pedgeBestErrorNL = pedge;
						pptBestErrorNL = ppt;
					}
				}else{
					if ( fBestError > fErrorBin ){
						fBestError = fErrorBin;
						pedgeBestError = pedge;
						pptBestError = ppt;
					}
				}
			}
		}
		fAverage /= (float)iAvCount;

		// Tweak up the NewLevel errors by a factor.
		if ( fBestError > ( fBestErrorNL + fAverage * m_fSlidingWindowErrorTolerance ) ){
			// Despite the boost, it's still the best,
			// so bite the bullet and do the collapse.
			fBestError = fBestErrorNL;
			pedgeBestError = pedgeBestErrorNL;
			pptBestError = pptBestErrorNL;
		}

		//-----------------------------------------------------------------------------------------------------------
		// Do we need to do any collapses?
		// Collapse auto-found edge.
		if ( ( pedgeBestError != NULL ) && ( pptBestError != NULL ) ){
			MeshPt *pKeptPt = pedgeBestError->OtherPt ( pptBestError ); 
			VERIFY ( pKeptPt != NULL );
			m_pObject->CreateEdgeCollapse ( pptBestError, pKeptPt );
		}else{
			break;
		}

		// max sliding window
		if (m_pObject->iCurSlidingWindowLevel>max_sliding_window) break;
	}
}

ETOOLS_API VIPM_Result*	 __stdcall VIPM_Convert		(u32 max_sliding_window, float error_tolerance, u32 optimize_vertex_order)
{
	g_pObject->Initialize	();
	if (!g_pObject->Valid())return NULL;
	CalculateAllCollapses	(g_pObject,max_sliding_window,error_tolerance);
	if (CalculateSW(g_pObject,g_pResult,optimize_vertex_order)) return g_pResult;
	else					return NULL;
}

ETOOLS_API void			 __stdcall VIPM_Destroy		()
{
//.	OutputDebugString	("VIPM_DESTROY-------------------\n");
	xr_delete			(g_pResult);
	xr_delete			(g_pObject);
	g_ppTempPts.resize	(0);
}
