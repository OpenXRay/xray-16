#include "pch.h"
#include "iostreams_proxy.h"

#ifdef _STLP_NO_IOSTREAMS

namespace std
{

console_output cout;
console_output cerr;
console_output clog;
char const *   endl = "\n";

}

#endif //#ifdef _STLP_NO_IOSTREAMS