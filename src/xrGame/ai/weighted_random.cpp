#include "StdAfx.h"
#include "weighted_random.h"
#include <math.h>

//------------------------------------------------------------
// weighted_random class
//------------------------------------------------------------

weighted_random::weighted_random()
{
    a_val = 0;
    a_weight = 1;
    b_val = 0;
    b_weight = -1;
    c_val = 0;
    c_weight = -1;
}

weighted_random::weighted_random(float val)
{
    a_val = val;
    a_weight = 1;
    b_val = 0;
    b_weight = -1;
    c_val = 0;
    c_weight = -1;
}

weighted_random::weighted_random(float av, float ap, float bv, float bp)
{
    a_val = av;
    a_weight = ap;
    b_val = bv;
    b_weight = bp;
    c_val = 0;
    c_weight = -1;
}

weighted_random::weighted_random(float av, float ap, float bv, float bp, float cv, float cp)
{
    a_val = av;
    a_weight = ap;
    b_val = bv;
    b_weight = bp;
    c_val = cv;
    c_weight = cp;
}

float weighted_random::generate()
{
    const float epsilon = 0.0001f;

    if (b_weight != -1 && c_weight != -1)
    {
        float ab_square = (a_weight + b_weight) * (b_val - a_val);
        float bc_square = (b_weight + c_weight) * (c_val - b_val);

        if (ab_square + bc_square < epsilon)
        {
            return a_val;
        }

        float random1 = (rand() % RAND_MAX) / (RAND_MAX - 1.f);
        float k1 = ab_square / (ab_square + bc_square);

        if (random1 < k1)
        {
            return weighted_random(a_val, a_weight, b_val, b_weight).generate();
        }
        else
        {
            return weighted_random(b_val, b_weight, c_val, c_weight).generate();
        }
    }
    else if (b_weight != -1)
    {
        float random1 = (rand() % RAND_MAX) / (RAND_MAX - 1.f);
        float random2 = (rand() % RAND_MAX) / (RAND_MAX - 1.f);

        float delta_weight = _abs(a_weight - b_weight);

        if (delta_weight < epsilon)
        {
            return a_val + (b_val - a_val) * random1;
        }
        else if (a_weight < b_weight)
        {
            float random_weight = (a_weight + b_weight) * random1;
            float random_val = a_val + (b_val - a_val) * random2;
            float k = delta_weight / (b_val - a_val);

            float weight_at = a_weight + (random_val - a_val) * k;

            if (random_weight < weight_at)
            {
                return random_val;
            }
            else
            {
                return a_val + b_val - random_val;
            }
        }
        else
        {
            float random_weight = (a_weight + b_weight) * random1;
            float random_val = a_val + (b_val - a_val) * random2;
            float k = delta_weight / (a_val - b_val);

            float weight_at = a_weight + (random_val - a_val) * k;

            if (random_weight < weight_at)
            {
                return random_val;
            }
            else
            {
                return a_val + b_val - random_val;
            }
        }
    }
    else
    {
        return a_val;
    }
}
