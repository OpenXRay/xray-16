#include "stdafx.h"

#include "fast_lc16.hpp"

#include <random>

static std::random_device s_random_device;

fast_lc16::fast_lc16()
{
    seed(s_random_device());
}

void fast_lc16::seed()
{
    seed(s_random_device());
}
