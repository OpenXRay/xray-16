#pragma once
 
#include "xrdeflectordefs.h"
#include "base_lighting.h"
#include "xrCDB/xrCDB.h"




class light_execute
{

		HASH			H;
		CDB::COLLIDER	DB;
		base_lighting	LightsSelected;
	 public:
		 void run( CDeflector& D );
};

