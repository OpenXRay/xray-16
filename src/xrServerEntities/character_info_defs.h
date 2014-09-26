#pragma once

#include "alife_space.h"


//личное отношение (благосклонность) одного персонажа к другому - 
//величина от -100< (крайне враждебное) до >100 (очень дрюжелюбное)
typedef int						CHARACTER_GOODWILL;
#define NO_GOODWILL				-type_max(CHARACTER_GOODWILL)
#define NEUTRAL_GOODWILL		CHARACTER_GOODWILL(0)

typedef shared_str				CHARACTER_CLASS;
#define NO_CHARACTER_CLASS		NULL

//репутация персонажа - величина от -100 (очень плохой, беспредельщик) 
//до 100 (очень хороший, благородный)
typedef int						CHARACTER_REPUTATION_VALUE;
#define NO_REPUTATION			-type_max(CHARACTER_REPUTATION_VALUE)
#define NEUTAL_REPUTATION		0

//ранг персонажа - величина от 0 (совсем неопытный) 
//до >100 (очень опытный)
typedef int						CHARACTER_RANK_VALUE;
#define NO_RANK					-type_max(CHARACTER_RANK_VALUE)


typedef shared_str				CHARACTER_COMMUNITY_ID;
#define NO_COMMUNITY_ID			CHARACTER_COMMUNITY_ID(NULL)

typedef int						CHARACTER_COMMUNITY_INDEX;
#define NO_COMMUNITY_INDEX		CHARACTER_COMMUNITY_INDEX(-1)