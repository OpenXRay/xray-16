#pragma once

#include "Common/Common.hpp"
#include "Common/FSMacros.hpp"
#include "xrCore/xrCore.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_string.h"

#include <msclr/marshal.h>

inline System::String^ BackSlashToSlash(pcstr originalString)
{
    System::String^ newString = gcnew System::String(originalString);
    return newString->Replace('\\', '/');
}
