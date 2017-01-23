#pragma once

#include "hash2D.h"
class  CDeflector;
typedef xr_vector<CDeflector*>			vecDefl			;
//extern vecDefl							g_deflectors	;
typedef vecDefl::iterator				vecDeflIt;
struct UVtri;
typedef hash2D<UVtri*,128,128>	HASH;