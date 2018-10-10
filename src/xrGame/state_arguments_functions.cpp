#include "StdAfx.h"
#include "state_arguments_functions.h"

namespace award_system
{
// definitions of static members of functions_cf
template<>
ge_function<float> functions_cf<float>::ge_function {};
template<>
le_function<float> functions_cf<float>::le_function {};

template<>
ge_function<u32> functions_cf<u32>::ge_function {};
template<>
le_function<u32> functions_cf<u32>::le_function {};

} // namespace award_system
