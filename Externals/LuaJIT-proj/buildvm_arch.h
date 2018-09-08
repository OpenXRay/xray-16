#if defined(_M_IX86)
#include "buildvm_arch_x86.h"
#elif defined(_M_X64)
#include "buildvm_arch_x64.h"
#else
#error Unsupported platform
#endif
