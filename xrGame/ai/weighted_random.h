#ifndef WEIGHTED_RANDOM
#define WEIGHTED_RANDOM

struct weighted_random
{
	float 		a_val;
	float 		a_weight;
	float 		b_val;
	float 		b_weight;
	float 		c_val;
	float 		c_weight;

	weighted_random	();
	weighted_random	(float val);
	weighted_random	(float av, float ap, float bv, float bp);
	weighted_random	(float av, float ap, float bv, float bp, float cv, float cp);

	bool			is_const	() { return b_weight == -1 && c_weight == -1; }
	float			generate	();
};

#endif // WEIGHTED_RANDOM