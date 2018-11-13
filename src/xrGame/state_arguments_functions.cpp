#include "StdAfx.h"
#include "state_arguments_functions.h"

namespace award_system
{
// definitions of static members of functions_cf
template<>
ge_function<float> functions_cf<float>::s_ge_function {};
template<>
le_function<float> functions_cf<float>::s_le_function {};

template<>
ge_function<u32> functions_cf<u32>::s_ge_function {};
template<>
le_function<u32> functions_cf<u32>::s_le_function {};

} // namespace award_system
