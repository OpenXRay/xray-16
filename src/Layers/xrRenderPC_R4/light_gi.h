#pragma once

struct	light_indirect		{
	Fvector			P;
	Fvector			D;
	float			E;
	IRender_Sector*	S;
};

