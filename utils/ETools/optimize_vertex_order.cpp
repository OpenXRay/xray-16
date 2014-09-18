#include "stdafx.h"
#pragma hdrstop

#include "ArbitraryList.h"
#include "BinaryHeap.h"

// The table of what score increase you get for reducing a valence to a certain amount.
// Any valence over the max value gets the max value boost.
// Bear in mind that three of these get added each tri.
const int c_iMaxValenceBoost = 5;
float fValenceBoost[c_iMaxValenceBoost] = 
{
	0.0f,
		0.68f,
		1.0f,
		1.1f,
		1.15f
};

int LOOKAHEAD = 5;
// Limit the number of tris to lookahead with.
int iLookaheadCutoff = 16;
// Limits how bad tris can be when adding to the heap.
const float fCutoffFactor = 2.0f;
// Another slightly different limit.
const float fExpensiveFactor = 1.5f;



// This stuff simulates a FIFO cache. There are lots of types, but (a) this is the
// easiest to model and (b) it's actually used in lots of real hardware (mainly
// because it's also the easiest to construct).


// Some data structures to track cache scores.

struct ScoreTri
{
	WORD	wIndex[3];
	bool	bUsed;
	bool	bAllowed;					// TRUE if we're even going to consider this tri.
};

struct ScoreVertex
{
	ArbitraryList<ScoreTri*> stri;		// The tris that use this vertex.
	int		iCurrentValence;			// Current unused valence of the vertex.
};

ArbitraryList<ScoreVertex> svVertex;
ArbitraryList<ScoreTri> stTri;
ArbitraryList<ScoreTri*> pstTri;

struct CacheTypeInfo
{
	// How long the FIFO is.
	int		iLength;
	// The score for a vertex miss.
	// If they all sum to 1.0f, you get a weighted average
	// of the number of vertices loaded coming out the end.
	float	fMissScore;
};


// The lengths of the various FIFOs and their relative weightings.
// These weightings are purely arbitrary - bias them to whichever
// your target hardware area is. You can usually find out FIFO
// lengths (or equivalent) from your friendly IHV devrel team.

#if 0

// General settings.
const int c_iCacheNumTypes = 4;
const CacheTypeInfo c_ctiCacheTypeSize[c_iCacheNumTypes] = 
{
	{  3, 0.15f }, 		// Sort of strip simulation. Sort of.
	{  4, 0.15f },		// Some hardware knows about quads.
	{ 12, 0.35f },		// Newer hardware is around 12-16. Heavier weighting on them.
	{ 16, 0.35f }
};

#elif 0

// Dedicated 12-entry cache.
const int c_iCacheNumTypes = 1;
const CacheTypeInfo c_ctiCacheTypeSize[c_iCacheNumTypes] = 
{
	{ 12, 1.0f }
};

#else

// 12-entry cache, with some weighting towards sharing that last tri.
// This slightly speeds up things that have a decent-sized secondary cache,
// but also a lightning-fast primary cache of the last tri.
// Only needs a tiny weighting to clean up the 12-entry-only stuff,
// which tends to look a bit fragmented, since it doesn't care where in
// the last 12 the used vertices are.
// Note that using this will increase the displayed "miss" count a bit, even
// though there are (usually) no extra misses for the second-level cache,
// and the result will still be optimal.
const int c_iCacheNumTypes = 2;
const CacheTypeInfo c_ctiCacheTypeSize[c_iCacheNumTypes] = 
{
	{ 20, 0.95f },
	{ 3, 0.05f }
};


#endif



struct CacheTypeTriInfo
{
	int		iIndicesAdded;		// Were 0, 1, 2 or 3 indices added to the start of the FIFO.
	WORD	wIndicesBinned[4];	// And the indices that fell off the end.
};


struct CacheTri
{
	WORD				wIndex[3];					// The tri added this time.
	CacheTypeTriInfo	ctti[c_iCacheNumTypes];		// The FIFO histories.
};


struct CacheVert
{
	BYTE				bRefcount[c_iCacheNumTypes];	// How many times each vertex is referenced by the cache.
};

ArbitraryList<CacheVert>	cvCacheStatus;
ArbitraryList<CacheTri>		ctCacheHistory;		// The cache history.

struct CurrentCacheState
{
	int iCacheCurrentMisses;		// The current number of vertex loads for each type.
	WORD *pwCacheFifoState;			// The current FIFO contents (circular buffer).
	int iCacheFifoWritePos;			// Where the current "tail" of the FIFO is.
} ccsState[c_iCacheNumTypes];


#ifdef _DEBUG
// Set this to 1 if you want to check the cache works, but it's pretty slow.
#define CACHE_DEBUG 0
#else
#define CACHE_DEBUG 0
#endif

void CacheInit ( int iNumberOfVertices )
{
	int i;
	for ( i = 0; i < c_iCacheNumTypes; i++ )
	{
		ccsState[i].iCacheCurrentMisses = 0;
		ccsState[i].pwCacheFifoState = new WORD [ c_ctiCacheTypeSize[i].iLength ];
		for ( int j = 0; j < c_ctiCacheTypeSize[i].iLength; j++ )
		{
			ccsState[i].pwCacheFifoState[j] = 0xFFFF;		// "unused" value - won't cause a cache hit.
		}
		ccsState[i].iCacheFifoWritePos = 0;
	}
	ctCacheHistory.resize ( 0 );
	cvCacheStatus.resize ( iNumberOfVertices );
	for ( i = 0; i < iNumberOfVertices; i++ )
	{
		for ( int j = 0; j < c_iCacheNumTypes; j++ )
		{
			(cvCacheStatus.item(i))->bRefcount[j] = 0;
		}
	}
}

void CacheBin ( void )
{
	for ( int i = 0; i < c_iCacheNumTypes; i++ )
	{
		VERIFY ( ccsState[i].pwCacheFifoState != NULL );
		delete[] ccsState[i].pwCacheFifoState;
		ccsState[i].pwCacheFifoState = NULL;
	}
	ctCacheHistory.resize ( 0 );
	cvCacheStatus.resize ( 0 );
}


// Returns the number of extra vertex misses due to this addition.
// Set bDontActuallyAdd to TRUE to just find the number of cache misses if this tri was added.
float CacheAddTri ( WORD wIndex1, WORD wIndex2, WORD wIndex3, BOOL bDontActuallyAdd = FALSE)
{
	WORD wIndex[3];
	wIndex[0] = wIndex1;
	wIndex[1] = wIndex2;
	wIndex[2] = wIndex3;

	CacheTri *pct = NULL;
	if ( !bDontActuallyAdd )
	{
		pct = ctCacheHistory.append();
		pct->wIndex[0] = wIndex[0];
		pct->wIndex[1] = wIndex[1];
		pct->wIndex[2] = wIndex[2];
	}

	float fTotalScore = 0.0f;
	for ( int iType = 0; iType < c_iCacheNumTypes; iType++ )
	{
		CurrentCacheState &ccs = ccsState[iType];

		int iCurNumMisses = ccs.iCacheCurrentMisses;

		// We must scan all three indices before adding any, otherwise you get nasty cases like:
		// 0,1,2 followed by 3,0,1. If you add as you go along, the 3 kicks out the 0,
		// then the 0 kicks out the 1, then the 1 kicks out the 2. And you have
		// 0 cache hits, rather than 2.
		// Most hardware does all three checks before adding as well.
		// NOTE! If your target hardware doesn't, shorten the FIFO a bit to get roughly the same behaviour.
		BOOL bFound[3];
		int iVert;
		for ( iVert = 0; iVert < 3; iVert++ )
		{
			bFound[iVert] = ( (cvCacheStatus.item(wIndex[iVert]))->bRefcount[iType] > 0 );

#if CACHE_DEBUG
			WORD wInd = wIndex[iVert];
			// Is this in the cache already?
			int j;
			BOOL bFoundThis = FALSE;
			//bFound[iVert] = FALSE;
			for ( j = c_ctiCacheTypeSize[iType].iLength - 1 ; j >= 0; j-- )
			{
				if ( ccs.pwCacheFifoState[j] == wInd )
				{
					// Found.
					//bFound[iVert] = TRUE;
					bFoundThis = TRUE;
					break;
				}
			}
			VERIFY ( bFoundThis == bFound[iVert] );
#endif
		}

		if ( bDontActuallyAdd )
		{
			// Just find what the cost is.
			for ( iVert = 0; iVert < 3; iVert++ )
			{
				if ( !bFound[iVert] )
				{
					fTotalScore += c_ctiCacheTypeSize[iType].fMissScore;
				}
			}
		}
		else
		{
			// Actually add the thing.
			pct->ctti[iType].iIndicesAdded = 0;
			for ( iVert = 0; iVert < 3; iVert++ )
			{
				if ( !bFound[iVert] )
				{
					// Not in the cache. Add it to the FIFO.
					WORD wInd = wIndex[iVert];

					// Store the current tail of the FIFO.
					pct->ctti[iType].wIndicesBinned[pct->ctti[iType].iIndicesAdded] = ccs.pwCacheFifoState[ccs.iCacheFifoWritePos];
					pct->ctti[iType].iIndicesAdded++;

					// This vert just fell off the end.
					WORD wOldInd = ccs.pwCacheFifoState[ccs.iCacheFifoWritePos];
					if ( wOldInd != 0xffff )
					{
						(cvCacheStatus.item(wOldInd))->bRefcount[iType]--;
						VERIFY ( (cvCacheStatus.item(wOldInd))->bRefcount[iType] == 0 );
					}


					// Put this index in.
					ccs.pwCacheFifoState[ccs.iCacheFifoWritePos] = wInd;

					// This vert is in.
					(cvCacheStatus.item(wInd))->bRefcount[iType]++;
					VERIFY ( (cvCacheStatus.item(wInd))->bRefcount[iType] == 1 );

					// Move the tail on.
					ccs.iCacheFifoWritePos++;
					if ( ccs.iCacheFifoWritePos >= c_ctiCacheTypeSize[iType].iLength )
					{
						ccs.iCacheFifoWritePos = 0;
					}

					// And we missed.
					ccs.iCacheCurrentMisses++;

					// And if this tri is degenerate, we need to spot that we've loaded the others.
					if ( iVert == 0 )
					{
						if ( wIndex[0] == wIndex[1] )
						{
							VERIFY ( !bFound[1] );
							bFound[1] = TRUE;
						}
						if ( wIndex[0] == wIndex[2] )
						{
							VERIFY ( !bFound[2] );
							bFound[2] = TRUE;
						}
					}
					else if ( iVert == 1 )
					{
						if ( wIndex[1] == wIndex[2] )
						{
							VERIFY ( !bFound[2] );
							bFound[2] = TRUE;
						}
					}
				}
				else
				{
					// Found. Do nothing.
				}
			}
			fTotalScore += c_ctiCacheTypeSize[iType].fMissScore * ( ccs.iCacheCurrentMisses - iCurNumMisses );
		}
	}

	return fTotalScore;
}

void CacheRemoveTri ( void )
{
	// Copy the last tri info.
	CacheTri ct = *(ctCacheHistory.item ( ctCacheHistory.size() - 1 ));
	// And bin the recorded one.
	ctCacheHistory.erase_fast( ctCacheHistory.size() - 1 );

	// Now undo the tri info.
	for ( int iType = 0; iType < c_iCacheNumTypes; iType++ )
	{
		CurrentCacheState &ccs = ccsState[iType];

		// Unroll the FIFO.
		for ( int i = ct.ctti[iType].iIndicesAdded - 1; i >= 0; i-- )
		{
			// Move the tail back.
			ccs.iCacheFifoWritePos--;
			if ( ccs.iCacheFifoWritePos < 0  )
			{
				ccs.iCacheFifoWritePos = c_ctiCacheTypeSize[iType].iLength - 1;
			}

			// This vert just fell off the start.
			WORD wOldInd = ccs.pwCacheFifoState[ccs.iCacheFifoWritePos];
			(cvCacheStatus.item(wOldInd))->bRefcount[iType]--;
			VERIFY ( (cvCacheStatus.item(wOldInd))->bRefcount[iType] == 0 );


			// Stuff this into the last entry in the fifo.
			WORD wInd = ct.ctti[iType].wIndicesBinned[i];
			ccs.pwCacheFifoState[ccs.iCacheFifoWritePos] = wInd;

			// Push this vert into the end.
			if ( wInd != 0xffff )
			{
				(cvCacheStatus.item(wInd))->bRefcount[iType]++;
				VERIFY ( (cvCacheStatus.item(wInd))->bRefcount[iType] == 1 );
			}


			// And we un-missed. :-)
			ccs.iCacheCurrentMisses--;
		}
	}
}

// Find the tri that was last added to the cache.
bool CacheGetLastTri ( WORD pwIndices[3] )
{
	if ( ctCacheHistory.size() > 0 )
	{
		CacheTri ct = *(ctCacheHistory.item ( ctCacheHistory.size() - 1 ));
		pwIndices[0] = ct.wIndex[0];
		pwIndices[1] = ct.wIndex[1];
		pwIndices[2] = ct.wIndex[2];
		return TRUE;
	}
	else
	{
		pwIndices[0] = -1;
		pwIndices[1] = -1;
		pwIndices[2] = -1;
		return FALSE;
	}
}

float GetNumVerticesLoadedTriList ( WORD *pwList, int iHowManyTris )
{
	// What's the top index?
	WORD wHighest = 0;
	int i;
	for ( i = 0; i < iHowManyTris * 3; i++ )
	{
		if ( wHighest < pwList[i] )
		{
			wHighest = pwList[i];
		}
	}

	CacheInit ( wHighest + 1 );

	// Let's find the current score of this list.
	float fScore = 0.0f;
	WORD *pwCurTri = pwList;
	for ( i = 0; i < iHowManyTris; i++ )
	{
		fScore += CacheAddTri ( pwCurTri[0], pwCurTri[1], pwCurTri[2] );
		pwCurTri += 3;
	}

	CacheBin();

	return fScore;
}

float FindBestScoreLookahead ( int iCountdown, float fCurrentScore, float fInputBestSoFar, 
							  int iNumTris, WORD *pwResult )
{

	VERIFY ( iCountdown <= LOOKAHEAD );

	int iLookahead = iCountdown;
	if ( iLookahead > LOOKAHEAD )
	{
		iLookahead = LOOKAHEAD;
	}


	float fBestSoFar = fInputBestSoFar;

	// Given the BestSoFar score, what is the average cost of each lookahead tri?
	float fAverageCost = ( fBestSoFar - fCurrentScore ) / (int)iLookahead;
	// And now allow tris that are a bit worse.
	float fExpensiveCutoff = fAverageCost * fExpensiveFactor;



	VERIFY ( iCountdown > 0 );
	VERIFY ( iNumTris > 0 );

	int j;



#if 0
	if ( iNumTris == 1 )
	{
		// Well, that's an easy decision then.

		// Find score after removal.
		//float fTriScore = CacheAddTri ( pwIndices[0], pwIndices[1], pwIndices[2], TRUE );
		ScoreTri *pstCur = ppstCurTris[0];
		float fTriScore = CacheAddTri ( pstCur->wIndex[0], pstCur->wIndex[1], pstCur->wIndex[2], TRUE );
		for ( j = 0; j < 3; j++ )
		{
			// Use the valence score after the tri has been removed.
			//int iValence = (*(iValenceCounts.item(pwIndices[j]))) - 1;
			int iValence = (svVertex.item(pstCur->wIndex[j])->iCurrentValence) - 1;
			VERIFY ( iValence >= 0 );
			if ( iValence < c_iMaxValenceBoost )
			{
				fTriScore += fValenceBoost[iValence];
			}
			else
			{
				fTriScore += fValenceBoost[c_iMaxValenceBoost-1];
			}
		}

		fTriScore += fCurrentScore;

		//pwResult[0] = pwIndices[0];
		//pwResult[1] = pwIndices[1];
		//pwResult[2] = pwIndices[2];
		pwResult[0] = pstCur->wIndex[0];
		pwResult[1] = pstCur->wIndex[1];
		pwResult[2] = pstCur->wIndex[2];

		if ( fTriScore > fBestSoFar )
		{
			// Oh well.
			// (actually, not sure this ever happens).
			return fInputBestSoFar;
		}
		else
		{
			return fTriScore;
		}
	}
#endif



#if 0
#ifdef _DEBUG
	for ( int k = 0; k < iNumTris; k++ )
	{
		VERIFY ( !ppstCurTris[k]->bAllowed );
		VERIFY ( !ppstCurTris[k]->bUsed );
	}
#endif
#endif


	// Should we limit ourselves to tris that share at least one vert with the previous one?
	bool bNoMadJumps = FALSE;
	WORD wPrevIndices[3];
	//if ( ( fBestSoFar - fCurrentScore ) < (float)iCountdown )
	// The lookahead score is lower than the countdown, so doing mad jumps
	// is not going to do sensible things.


	ArbitraryList<ScoreTri*> pstTriLocal;

	//if ( iCountdown == iLookahead )
	{
		// Ah - this is probably a lookahead, not assembling the real list.
		// Find the previous indices from the cache (a bit of a roundabout method).
		int iNumAllowed = 0;
		BOOL bRes = CacheGetLastTri ( wPrevIndices );
		VERIFY ( bRes );
		{
			bNoMadJumps = TRUE;
			// And mark all the tris that these used as "allowed"
			for ( int i = 0; i < 3; i++ )
			{
				ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
				for ( int j = 0; j < psv->stri.size(); j++ )
				{
					ScoreTri *pst = (*(psv->stri.item(j)));
					if ( ( !pst->bUsed ) && ( !pst->bAllowed ) )
					{
						pst->bAllowed = TRUE;
						*(pstTriLocal.append()) = pst;
						iNumAllowed++;
					}
				}
			}

			if ( iNumAllowed < 1 )
			{
				// Oops - we've gone into a dead-end, so that very few tris
				// are allowed. So open the selection up again.
				pstTriLocal.resize(0);
				bNoMadJumps = FALSE;
				int i;
				for ( i = 0; i < 3; i++ )
				{
					ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
					for ( int j = 0; j < psv->stri.size(); j++ )
					{
						ScoreTri *pst = (*(psv->stri.item(j)));
						if ( !pst->bUsed )
						{
							pst->bAllowed = FALSE;
						}
					}
				}

				for ( i = 0; i < stTri.size(); i++ )
				{
					if ( !(stTri.item(i))->bUsed )
					{
						*(pstTriLocal.append()) = stTri.item(i);
					}
				}
			}
		}
	}


	// Add the tris to a new priority queue, sorting by score.
	//BinaryHeap<WORD, float> NewHeap;
	BinaryHeap<ScoreTri, float> NewHeap;

	int i;
	//WORD *pwCurIndex = pwIndices;
	for ( i = pstTriLocal.size() - 1; i >= 0; i-- )
	{
		ScoreTri *pstCurTri = *(pstTriLocal.item(i));
		float fTriScore = 0.0f;
		if ( bNoMadJumps )
		{
			if ( !pstCurTri->bAllowed )
			{
				continue;
			}

#if IGNORE_VALENCE_FOR_LOOKAHEADS
			// Only use the valence stuff on non-lookaheads.
			// To make the scores comparable, add the maximum valence boost all the time.
			fTriScore += 3.0f * fValenceBoost[c_iMaxValenceBoost-1];
#endif
		}
#if IGNORE_VALENCE_FOR_LOOKAHEADS
		else
#endif
		{
			// Only use the valence stuff on non-lookaheads.
			for ( j = 0; j < 3; j++ )
			{
				// Use the valence score after the tri has been removed.
				//int iValence = (*(iValenceCounts.item(pwCurIndex[j]))) - 1;
				int iValence = (svVertex.item(pstCurTri->wIndex[j])->iCurrentValence) - 1;
				VERIFY ( iValence >= 0 );
				if ( iValence < c_iMaxValenceBoost )
				{
					fTriScore += fValenceBoost[iValence];
				}
				else
				{
					fTriScore += fValenceBoost[c_iMaxValenceBoost-1];
				}
			}
		}

		if ( fTriScore > fExpensiveCutoff )
		{
			// This tri is a lot more expensive than the average cost of the BestSoFar tris.
			// It's very unlikely to give us a good result.
			continue;
		}

		if ( fCurrentScore + fTriScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			//pwCurIndex += 3;
			continue;
		}


		// And the vertex cost.
		fTriScore += CacheAddTri ( pstCurTri->wIndex[0], pstCurTri->wIndex[1], pstCurTri->wIndex[2], TRUE );

		if ( fTriScore > fExpensiveCutoff )
		{
			// This tri is a lot more expensive than the average cost of the BestSoFar tris.
			// It's very unlikely to give us a good result.
			continue;
		}

		if ( fCurrentScore + fTriScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			//pwCurIndex += 3;
			continue;
		}

		// And bung it in the heap.
		// We -ve the score so that the first in the heap is the one with the _least_ score.
		NewHeap.Add ( pstCurTri, -fTriScore );
		//pwCurIndex += 3;
	}



	// Undo the "allowed" flags.
	if ( bNoMadJumps )
	{
		for ( int i = 0; i < 3; i++ )
		{
			ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
			for ( int j = 0; j < psv->stri.size(); j++ )
			{
				(*(psv->stri.item(j)))->bAllowed = FALSE;
			}
		}
	}


	// Now extract from the heap, best score to worst.
	// This attempts to get early-outs very quickly and prevent too much recursion.
	//WORD *pwBest = NULL;
	//WORD *pwCur = NewHeap.FindFirst();
	ScoreTri *pstBest = NULL;
	ScoreTri *pstCur = NewHeap.FindFirst();

	//if ( pwCur == NULL )
	if ( pstCur == NULL )
	{
		// Found nothing that was better.
		return fBestSoFar;
	}

	// Above this score, just don't bother.
	float fCutoffScore = fCutoffFactor * NewHeap.GetCurrentSort();

#ifdef _DEBUG
	float fPrevScore = 1.0f;
#endif

	int iTried = 0;
	//while ( pwCur != NULL )
	while ( pstCur != NULL )
	{
		float fThisTriNegScore = NewHeap.GetCurrentSort();
		NewHeap.RemoveFirst();

#ifdef _DEBUG
		// Check this heap actually works!
		VERIFY ( fThisTriNegScore <= fPrevScore );
		fPrevScore = fThisTriNegScore;
#endif


		if ( fThisTriNegScore < fCutoffScore )
		{
			// Reached the cutoff for this tri - don't bother continuing.
			break;
		}

		float fScore = fCurrentScore - fThisTriNegScore;

		if ( fScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			// The reast of the heap is bound to be greater, so don't bother continuing.
			break;
		}

		if ( bNoMadJumps )
		{
			iTried++;
			if ( iTried > iLookaheadCutoff )
			{
				// Tried enough tris - don't want to cause a combinatorial explosion.
				break;
			}
		}

		// Do the valencies.
#ifdef _DEBUG
		float fValenceScore = 0.0f;
#endif
#if IGNORE_VALENCE_FOR_LOOKAHEADS
		if ( bNoMadJumps )
		{
			// Only use the valence stuff on non-lookaheads.
			// To make the scores comparable, add the maximum valence boost all the time.
			fValenceScore = 3.0f * fValenceBoost[c_iMaxValenceBoost-1];
		}
		else
#endif
		{
			for ( j = 0; j < 3; j++ )
			{
				//int iValence = --(*(iValenceCounts.item(pwCur[j])));
				int iValence = --(svVertex.item(pstCur->wIndex[j])->iCurrentValence);
#ifdef _DEBUG
				VERIFY ( iValence >= 0 );
				if ( iValence < c_iMaxValenceBoost )
				{
					fValenceScore += fValenceBoost[iValence];
				}
				else
				{
					fValenceScore += fValenceBoost[c_iMaxValenceBoost-1];
				}
#endif
			}
		}

		// Add it to the cache.
		float fScoreTemp = CacheAddTri ( pstCur->wIndex[0], pstCur->wIndex[1], pstCur->wIndex[2] );

		VERIFY ( !pstCur->bUsed );
		pstCur->bUsed = TRUE;


#ifdef _DEBUG
		fScoreTemp += fValenceScore;
		VERIFY ( fabs ( fScoreTemp + fThisTriNegScore ) < 0.0001f );
#endif

		if ( iLookahead > 1 )
		{
			// Swap pwCur to the start of the list.
			// And look ahead a bit more.
			//fScore = FindBestScoreLookahead ( iLookahead - 1, fScore, fBestSoFar, pwIndices + 3, iNumTris - 1, pwResult + 3 );
			float fNewScore = FindBestScoreLookahead ( iLookahead - 1, fScore, fBestSoFar, iNumTris - 1, pwResult + 3 );
			//VERIFY ( fNewScore < 1e9 );
			fScore = fNewScore;
		}
		//VERIFY ( fScore < 1e9 );
		if ( fScore < fBestSoFar )
		{
			fBestSoFar = fScore;
			//pwBest = pwCur;
			pstBest = pstCur;
		}

		CacheRemoveTri();
		VERIFY ( pstCur->bUsed );
		pstCur->bUsed = FALSE;

#if IGNORE_VALENCE_FOR_LOOKAHEADS
		if ( !bNoMadJumps )
#endif
		{
			// Restore the valencies.
			for ( j = 0; j < 3; j++ )
			{
				//++(*(iValenceCounts.item(pwCur[j])));
				++(svVertex.item(pstCur->wIndex[j])->iCurrentValence);
			}
		}

		//pwCur = NewHeap.FindFirst();
		pstCur = NewHeap.FindFirst();
	}


	//if ( pwBest == NULL )
	if ( pstBest == NULL )
	{
		// We didn't find a solution that was better than the existing one.
		//VERIFY ( fBestSoFar < 1e9 );
		return fInputBestSoFar;
	}
	else
	{
		// Found a better solution.


		// Swap the best tri to the start of the list.
		float fNewBestScore = fBestSoFar;

		// Dump the tri to the result buffer.
		//ScoreTri *pstBest = *ppstBest;
		//pwResult[0] = pwBest[0];
		//pwResult[1] = pwBest[1];
		//pwResult[2] = pwBest[2];
		pwResult[0] = pstBest->wIndex[0];
		pwResult[1] = pstBest->wIndex[1];
		pwResult[2] = pstBest->wIndex[2];
		if ( iCountdown > 1 )
		{
			//float fScore = CacheAddTri ( wTemp[0], wTemp[1], wTemp[2] );
			float fScore = CacheAddTri ( pstBest->wIndex[0], pstBest->wIndex[1], pstBest->wIndex[2] );
			VERIFY ( !pstBest->bUsed );
			pstBest->bUsed = TRUE;

			// And the valence.
			float fValenceScore = 0.0f;
#if IGNORE_VALENCE_FOR_LOOKAHEADS
			//if ( !bNoMadJumps )
#endif
			{
				for ( j = 0; j < 3; j++ )
				{
					//int iValence = --(*(iValenceCounts.item(wTemp[j])));
					int iValence = --(svVertex.item(pstBest->wIndex[j])->iCurrentValence);
					VERIFY ( iValence >= 0 );
					if ( iValence < c_iMaxValenceBoost )
					{
						fValenceScore += fValenceBoost[iValence];
					}
					else
					{
						fValenceScore += fValenceBoost[c_iMaxValenceBoost-1];
					}
				}
			}

			fScore += fValenceScore + fCurrentScore;

#ifdef DEBUG
			// Keep track of our progress.
			if ( iCountdown > LOOKAHEAD )
			{
				// Ah - we must be assembling the real list.
				//.				TRACE ( "Countdown %i\n", iCountdown );
			}
#endif


			//VERIFY ( fScore < 1e9 );

			//fNewBestScore = FindBestScoreLookahead ( iCountdown - 1, fScore, 1e10, pwIndices + 3, iNumTris - 1, pwResult + 3 );
			fNewBestScore = FindBestScoreLookahead ( iCountdown - 1, fScore, 1e10, iNumTris - 1, pwResult + 3 );

			//VERIFY ( fNewBestScore < 1e9 );

			CacheRemoveTri();
			VERIFY ( pstBest->bUsed );
			pstBest->bUsed = FALSE;

			// Restore the valencies.
#if IGNORE_VALENCE_FOR_LOOKAHEADS
			//if ( !bNoMadJumps )
#endif
			{
				for ( j = 0; j < 3; j++ )
				{
					//++(*(iValenceCounts.item(wTemp[j])));
					++(svVertex.item(pstBest->wIndex[j])->iCurrentValence);
				}
			}
		}

		return fNewBestScore;
	}
}

float FindBestScore ( int iCountdown, float fCurrentScore, float fInputBestSoFar, 
					 //WORD *pwIndices,
					 ScoreTri **ppstCurTris,		// Pointer to a list of pointers to ScoreTris
					 int iNumTris, WORD *pwResult )


{
	int iLookahead = iCountdown;
	if ( iLookahead > LOOKAHEAD )
	{
		iLookahead = LOOKAHEAD;
	}

	//VERIFY ( fCurrentScore < 1e9 );


	// At the start, limit the lookahead, or it takes ages.
	if ( ( iNumTris > 100 ) && ( fCurrentScore < 50.0f ) && ( iLookahead > 2 ) )
	{
		iLookahead = 2;
	}


	float fBestSoFar = fInputBestSoFar;

	// Given the BestSoFar score, what is the average cost of each lookahead tri?
	float fAverageCost = ( fBestSoFar - fCurrentScore ) / (int)iLookahead;
	// And now allow tris that are a bit worse.
	float fExpensiveCutoff = fAverageCost * fExpensiveFactor;



	VERIFY ( iCountdown > 0 );
	VERIFY ( iNumTris > 0 );

	int j;



	if ( iNumTris == 1 )
	{
		// Well, that's an easy decision then.

		// Find score after removal.
		//float fTriScore = CacheAddTri ( pwIndices[0], pwIndices[1], pwIndices[2], TRUE );
		ScoreTri *pstCur = ppstCurTris[0];
		float fTriScore = CacheAddTri ( pstCur->wIndex[0], pstCur->wIndex[1], pstCur->wIndex[2], TRUE );
		for ( j = 0; j < 3; j++ )
		{
			// Use the valence score after the tri has been removed.
			//int iValence = (*(iValenceCounts.item(pwIndices[j]))) - 1;
			int iValence = (svVertex.item(pstCur->wIndex[j])->iCurrentValence) - 1;
			VERIFY ( iValence >= 0 );
			if ( iValence < c_iMaxValenceBoost )
			{
				fTriScore += fValenceBoost[iValence];
			}
			else
			{
				fTriScore += fValenceBoost[c_iMaxValenceBoost-1];
			}
		}

		fTriScore += fCurrentScore;

		//pwResult[0] = pwIndices[0];
		//pwResult[1] = pwIndices[1];
		//pwResult[2] = pwIndices[2];
		pwResult[0] = pstCur->wIndex[0];
		pwResult[1] = pstCur->wIndex[1];
		pwResult[2] = pstCur->wIndex[2];

		if ( fTriScore > fBestSoFar )
		{
			// Oh well.
			// (actually, not sure this ever happens).
			return fInputBestSoFar;




		}
		else
		{
			return fTriScore;
		}
	}



#ifdef _DEBUG
	for ( int k = 0; k < iNumTris; k++ )
	{
		VERIFY ( !ppstCurTris[k]->bAllowed );
		VERIFY ( !ppstCurTris[k]->bUsed );
	}
#endif


	// Should we limit ourselves to tris that share at least one vert with the previous one?
	bool bNoMadJumps = FALSE;
	WORD wPrevIndices[3];
	//if ( ( fBestSoFar - fCurrentScore ) < (float)iCountdown )
	// The lookahead score is lower than the countdown, so doing mad jumps
	// is not going to do sensible things.
	if ( iCountdown == iLookahead )
	{
		// Ah - this is probably a lookahead, not assembling the real list.
		// Find the previous indices from the cache (a bit of a roundabout method).
		int iNumAllowed = 0;
		if ( CacheGetLastTri ( wPrevIndices ) )
		{
			bNoMadJumps = TRUE;
			// And mark all the tris that these used as "allowed"
			for ( int i = 0; i < 3; i++ )
			{
				ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
				for ( int j = 0; j < psv->stri.size(); j++ )
				{
					ScoreTri *pst = (*(psv->stri.item(j)));
					if ( ( !pst->bUsed ) && ( !pst->bAllowed ) )
					{
						pst->bAllowed = TRUE;
						iNumAllowed++;
					}
				}
			}
			if ( iNumAllowed < 1 )
			{
				// Oops - we've probably gone into a dead-end, so that very few tris
				// are allowed. So open the selection up again.
				bNoMadJumps = FALSE;
				for ( int i = 0; i < 3; i++ )
				{
					ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
					for ( int j = 0; j < psv->stri.size(); j++ )
					{
						ScoreTri *pst = (*(psv->stri.item(j)));
						if ( !pst->bUsed )
						{
							pst->bAllowed = FALSE;
						}
					}
				}
			}
		}
	}


	// Add the tris to a new priority queue, sorting by score.
	//BinaryHeap<WORD, float> NewHeap;
	BinaryHeap<ScoreTri*, float> NewHeap;

	int i;
	//WORD *pwCurIndex = pwIndices;
	for ( i = 0; i < iNumTris; i++ )
	{
		ScoreTri *pstCurTri = ppstCurTris[i];
		float fTriScore = 0.0f;
		if ( bNoMadJumps )
		{
			if ( !pstCurTri->bAllowed )
			{
				continue;
			}

#if IGNORE_VALENCE_FOR_LOOKAHEADS
			// Only use the valence stuff on non-lookaheads.
			// To make the scores comparable, add the maximum valence boost all the time.
			fTriScore += 3.0f * fValenceBoost[c_iMaxValenceBoost-1];
#endif
		}
#if IGNORE_VALENCE_FOR_LOOKAHEADS
		else
#endif
		{
			// Only use the valence stuff on non-lookaheads.
			for ( j = 0; j < 3; j++ )
			{
				// Use the valence score after the tri has been removed.
				//int iValence = (*(iValenceCounts.item(pwCurIndex[j]))) - 1;
				int iValence = (svVertex.item(pstCurTri->wIndex[j])->iCurrentValence) - 1;
				VERIFY ( iValence >= 0 );
				if ( iValence < c_iMaxValenceBoost )
				{
					fTriScore += fValenceBoost[iValence];
				}
				else
				{
					fTriScore += fValenceBoost[c_iMaxValenceBoost-1];
				}
			}
		}

		if ( fTriScore > fExpensiveCutoff )
		{
			// This tri is a lot more expensive than the average cost of the BestSoFar tris.
			// It's very unlikely to give us a good result.
			continue;
		}

		if ( fCurrentScore + fTriScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			//pwCurIndex += 3;
			continue;
		}


		// And the vertex cost.
		fTriScore += CacheAddTri ( pstCurTri->wIndex[0], pstCurTri->wIndex[1], pstCurTri->wIndex[2], TRUE );

		if ( fTriScore > fExpensiveCutoff )
		{
			// This tri is a lot more expensive than the average cost of the BestSoFar tris.
			// It's very unlikely to give us a good result.
			continue;
		}

		if ( fCurrentScore + fTriScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			//pwCurIndex += 3;
			continue;
		}

		// And bung it in the heap.
		// We -ve the score so that the first in the heap is the one with the _least_ score.
		NewHeap.Add ( &(ppstCurTris[i]), -fTriScore );
		//pwCurIndex += 3;
	}



	// Undo the "allowed" flags.
	if ( bNoMadJumps )
	{
		for ( int i = 0; i < 3; i++ )
		{
			ScoreVertex *psv = svVertex.item(wPrevIndices[i]);
			for ( int j = 0; j < psv->stri.size(); j++ )
			{
				(*(psv->stri.item(j)))->bAllowed = FALSE;
			}
		}
	}

#ifdef _DEBUG
	for ( k = 0; k < iNumTris; k++ )
	{
		VERIFY ( !ppstCurTris[k]->bAllowed );
		VERIFY ( !ppstCurTris[k]->bUsed );
	}
#endif



	// Now extract from the heap, best score to worst.
	// This attempts to get early-outs very quickly and prevent too much recursion.
	//WORD *pwBest = NULL;
	//WORD *pwCur = NewHeap.FindFirst();
	ScoreTri **ppstBest = NULL;
	ScoreTri **ppstCur = NewHeap.FindFirst();

	//if ( pwCur == NULL )
	if ( ppstCur == NULL )
	{
		// Found nothing that was better.
		return fBestSoFar;
	}

	// Above this score, just don't bother.
	float fCutoffScore = fCutoffFactor * NewHeap.GetCurrentSort();

#ifdef _DEBUG
	float fPrevScore = 1.0f;
#endif

	int iTried = 0;
	//while ( pwCur != NULL )
	while ( ppstCur != NULL )
	{
		ScoreTri *pstCur = *ppstCur;

		float fThisTriNegScore = NewHeap.GetCurrentSort();
		NewHeap.RemoveFirst();

#ifdef _DEBUG
		// Check this heap actually works!
		VERIFY ( fThisTriNegScore <= fPrevScore );
		fPrevScore = fThisTriNegScore;
#endif


		if ( fThisTriNegScore < fCutoffScore )
		{
			// Reached the cutoff for this tri - don't bother continuing.
			break;
		}

		float fScore = fCurrentScore - fThisTriNegScore;

		if ( fScore >= fBestSoFar )
		{
			// We've already gone more than the best score.
			// The reast of the heap is bound to be greater, so don't bother continuing.
			break;
		}

		if ( bNoMadJumps )
		{
			iTried++;
			if ( iTried > iLookaheadCutoff )
			{
				// Tried enough tris - don't want to cause a combinatorial explosion.
				break;
			}
		}

		// Do the valencies.
#ifdef _DEBUG
		float fValenceScore = 0.0f;
#endif
#if IGNORE_VALENCE_FOR_LOOKAHEADS
		if ( bNoMadJumps )
		{
			// Only use the valence stuff on non-lookaheads.
			// To make the scores comparable, add the maximum valence boost all the time.
			fValenceScore = 3.0f * fValenceBoost[c_iMaxValenceBoost-1];
		}
		else
#endif
		{
			for ( j = 0; j < 3; j++ )
			{
				//int iValence = --(*(iValenceCounts.item(pwCur[j])));
				int iValence = --(svVertex.item(pstCur->wIndex[j])->iCurrentValence);
#ifdef _DEBUG
				VERIFY ( iValence >= 0 );
				if ( iValence < c_iMaxValenceBoost )
				{
					fValenceScore += fValenceBoost[iValence];
				}
				else
				{
					fValenceScore += fValenceBoost[c_iMaxValenceBoost-1];
				}
#endif
			}
		}

		// Add it to the cache.
		float fScoreTemp = CacheAddTri ( pstCur->wIndex[0], pstCur->wIndex[1], pstCur->wIndex[2] );

		VERIFY ( !pstCur->bUsed );
		pstCur->bUsed = TRUE;


#ifdef _DEBUG
		fScoreTemp += fValenceScore;
		VERIFY ( fabs ( fScoreTemp + fThisTriNegScore ) < 0.0001f );
#endif

		if ( iLookahead > 1 )
		{
			// Swap pwCur to the start of the list.
#if 0
			WORD wTemp0 = pwCur[0];
			WORD wTemp1 = pwCur[1];
			WORD wTemp2 = pwCur[2];
			pwCur[0] = pwIndices[0];
			pwCur[1] = pwIndices[1];
			pwCur[2] = pwIndices[2];
			//pwIndices[0] = wTemp0;
			//pwIndices[1] = wTemp1;
			//pwIndices[2] = wTemp2;
#else
			ScoreTri *pstTemp = *ppstCur;
			*ppstCur = *ppstCurTris;
			//*ppstCurTris = pstTemp;
#endif

			//VERIFY ( fScore < 1e9 );

			// And look ahead a bit more.
			//fScore = FindBestScoreLookahead ( iLookahead - 1, fScore, fBestSoFar, pwIndices + 3, iNumTris - 1, pwResult + 3 );
			float fNewScore = FindBestScoreLookahead ( iLookahead - 1, fScore, fBestSoFar, iNumTris - 1, pwResult + 3 );
			//VERIFY ( fNewScore < 1e9 );
			fScore = fNewScore;

#if 0
			// And swap it back.
			//wTemp0 = pwIndices[0];
			//wTemp1 = pwIndices[1];
			//wTemp2 = pwIndices[2];
			pwIndices[0] = pwCur[0];
			pwIndices[1] = pwCur[1];
			pwIndices[2] = pwCur[2];
			pwCur[0] = wTemp0;
			pwCur[1] = wTemp1;
			pwCur[2] = wTemp2;
#else
			//pstTemp = *ppstCurTris;
			*ppstCurTris = *ppstCur;
			*ppstCur = pstTemp;
#endif
		}
		//VERIFY ( fScore < 1e9 );
		if ( fScore < fBestSoFar )
		{
			fBestSoFar = fScore;
			//pwBest = pwCur;
			ppstBest = ppstCur;
		}

		CacheRemoveTri();
		VERIFY ( pstCur->bUsed );
		pstCur->bUsed = FALSE;

#if IGNORE_VALENCE_FOR_LOOKAHEADS
		if ( !bNoMadJumps )
#endif
		{
			// Restore the valencies.
			for ( j = 0; j < 3; j++ )
			{
				//++(*(iValenceCounts.item(pwCur[j])));
				++(svVertex.item(pstCur->wIndex[j])->iCurrentValence);
			}
		}

		//pwCur = NewHeap.FindFirst();
		ppstCur = NewHeap.FindFirst();
	}


















	VERIFY ( ppstBest != NULL );
	{
		// Found a better solution.











		// Swap the best tri to the start of the list.
		float fNewBestScore = fBestSoFar;


		// Dump the tri to the result buffer.
		ScoreTri *pstBest = *ppstBest;
		//pwResult[0] = pwBest[0];
		//pwResult[1] = pwBest[1];
		//pwResult[2] = pwBest[2];
		pwResult[0] = pstBest->wIndex[0];
		pwResult[1] = pstBest->wIndex[1];
		pwResult[2] = pstBest->wIndex[2];
		if ( iCountdown > 1 )
		{
#if 0
			WORD wTemp[3];
			wTemp[0] = pwBest[0];
			wTemp[1] = pwBest[1];
			wTemp[2] = pwBest[2];
			pwBest[0] = pwIndices[0];
			pwBest[1] = pwIndices[1];
			pwBest[2] = pwIndices[2];		
#else
			ScoreTri *pTemp;
			pTemp = *ppstBest;
			*ppstBest = *ppstCurTris;
#endif

			//float fScore = CacheAddTri ( wTemp[0], wTemp[1], wTemp[2] );
			float fScore = CacheAddTri ( pstBest->wIndex[0], pstBest->wIndex[1], pstBest->wIndex[2] );
			VERIFY ( !pstBest->bUsed );
			pstBest->bUsed = TRUE;

			// And the valence.
			float fValenceScore = 0.0f;
#if IGNORE_VALENCE_FOR_LOOKAHEADS
			//if ( !bNoMadJumps )
#endif
			{
				for ( j = 0; j < 3; j++ )
				{
					//int iValence = --(*(iValenceCounts.item(wTemp[j])));
					int iValence = --(svVertex.item(pstBest->wIndex[j])->iCurrentValence);
					VERIFY ( iValence >= 0 );
					if ( iValence < c_iMaxValenceBoost )
					{
						fValenceScore += fValenceBoost[iValence];
					}
					else
					{
						fValenceScore += fValenceBoost[c_iMaxValenceBoost-1];
					}
				}
			}

			fScore += fValenceScore + fCurrentScore;

#ifdef DEBUG
			//.			TRACE ( "Countdown %i\n", iCountdown );
#endif


#if ALLOW_PROGRESS_BARS
			if ( g_hProgress2 != NULL )
			{
				// Step one.
				SendMessage ( g_hProgress2, PBM_STEPIT, 0, 0 );
			}
#endif


			//VERIFY ( fScore < 1e9 );

			//fNewBestScore = FindBestScore ( iCountdown - 1, fScore, 1e10, pwIndices + 3, iNumTris - 1, pwResult + 3 );
			fNewBestScore = FindBestScore ( iCountdown - 1, fScore, 1e10, ppstCurTris + 1, iNumTris - 1, pwResult + 3 );

			//VERIFY ( fNewBestScore < 1e9 );



			CacheRemoveTri();
			VERIFY ( pstBest->bUsed );
			pstBest->bUsed = FALSE;

			// Restore the valencies.
#if IGNORE_VALENCE_FOR_LOOKAHEADS
			//if ( !bNoMadJumps )
#endif
			{
				for ( j = 0; j < 3; j++ )
				{
					//++(*(iValenceCounts.item(wTemp[j])));
					++(svVertex.item(pstBest->wIndex[j])->iCurrentValence);
				}
			}

			// And swap it back.
#if 0
			pwIndices[0] = pwBest[0];
			pwIndices[1] = pwBest[1];
			pwIndices[2] = pwBest[2];
			pwBest[0] = wTemp[0];
			pwBest[1] = wTemp[1];
			pwBest[2] = wTemp[2];
#else
			*ppstCurTris = *ppstBest;
			*ppstBest = pTemp;
#endif



		}

		return fNewBestScore;


	}


}

// Call this to reorder the tris in this trilist to get good vertex-cache coherency.
// *pwList is modified (but obviously not changed in size or memory location).
void _OptimiseVertexCoherencyTriList ( WORD *pwList, int iHowManyTris, u32 optimize_mode)
{
	if ( iHowManyTris <= 2 )
	{
		// Silly.
		return;
	}

	switch (optimize_mode){
case 1: 
	// Very cheap'n'cheerful.
	LOOKAHEAD = 1;
	iLookaheadCutoff = 4;
	break;
case 2: 
	// Expensive, but better.
	LOOKAHEAD = 5;
	iLookaheadCutoff = 16;
	break;
	}

#ifdef _DEBUG
	// Find current score (probably rubbish).
	float fCurrentScore = GetNumVerticesLoadedTriList ( pwList, iHowManyTris );
#endif

	// First scan to find the biggest index.
	WORD *pwIndex = pwList;
	int iBiggestIndex = 0;
	int i;
	for ( i = 0; i < iHowManyTris * 3; i++ )
	{
		if ( iBiggestIndex < *pwIndex )
		{
			iBiggestIndex = *pwIndex;
		}
		pwIndex++;
	}

#if 0
	iValenceCounts.resize(iBiggestIndex+1);
	for ( i = 0; i <= iBiggestIndex; i++ )
	{
		*(iValenceCounts.item(i)) = 0;
	}


	// And set up the valencies.
	pwIndex = pwList;
	for ( i = 0; i < iHowManyTris * 3; i++ )
	{
		++(*(iValenceCounts.item(*pwIndex)));
		pwIndex++;
	}
#else

	svVertex.resize ( iBiggestIndex + 1 );
	for ( i = 0; i <= iBiggestIndex; i++ )
	{
		ScoreVertex *sv = svVertex.item(i);
		sv->iCurrentValence = 0;
		sv->stri.resize(0);
	}

	stTri.resize ( iHowManyTris );
	pstTri.resize ( iHowManyTris );
	WORD *pwCurTri = pwList;
	for ( i = 0; i < iHowManyTris; i++ )
	{
		ScoreTri *pstCur = stTri.item(i);
		*(pstTri.item(i)) = pstCur;
		pstCur->bUsed = FALSE;
		pstCur->bAllowed = FALSE;
		for ( int j = 0; j < 3; j++ )
		{
			pstCur->wIndex[j] = pwCurTri[j];
			svVertex.item(pwCurTri[j])->iCurrentValence++;
			*(svVertex.item(pwCurTri[j])->stri.append()) = pstCur;
		}
		pwCurTri += 3;
	}

#endif

	WORD *pwNewIndex = new WORD[iHowManyTris*3];

	CacheInit ( iBiggestIndex + 1 );


#if 0
	// If you try to optimise from the very first tri, it takes ages to find the "best"
	// first tri - almost none of the early-outs work, because there is no cache history,
	// and indeed the whole concept of a "best" first tri is madness - you have to start
	// somewhere.
	// So I just pick the first one, and let it carry on from there. Once the cache has
	// been "seeded" the early-outs come thick and fast and it's a decent speed.
	pwNewIndex[0] = pwList[0];
	pwNewIndex[1] = pwList[1];
	pwNewIndex[2] = pwList[2];
	--(*(iValenceCounts.item(pwList[0])));
	--(*(iValenceCounts.item(pwList[1])));
	--(*(iValenceCounts.item(pwList[2])));
	CacheAddTri ( pwList[0], pwList[1], pwList[2] );
	// No need to actually increase the score - everything is relative anyway.

	float fTemp = FindBestScore (	iHowManyTris - 1,
		0.0f,
		1e10,
		pwList + 3,
		iHowManyTris - 1,
		pwNewIndex + 3 );
#else
	float fTemp = FindBestScore (	iHowManyTris,
		0.0f,
		1e10,
		(pstTri.item(0)),
		iHowManyTris,
		pwNewIndex );
#endif

	CacheBin();

#if 0
	iValenceCounts.resize(0);
#else
	svVertex.resize(0);
	stTri.resize(0);
#endif


#ifdef _DEBUG
	// Find new score (hopefully better).
	float fNewScore = GetNumVerticesLoadedTriList ( pwNewIndex, iHowManyTris );
	fNewScore /= (float)iHowManyTris;		// The verts per tri score.
	// And if you definately use indices 0 to iBiggestIndex (which all this code does), then this is the max possible:
	float fMaxPossible = iBiggestIndex / (float)iHowManyTris;
#endif


	memcpy ( pwList, pwNewIndex, sizeof(WORD) * 3 * iHowManyTris );

	delete[] pwNewIndex;
}
