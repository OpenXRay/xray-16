#include "stdafx.h"
#include "DeveloperMode.h"

bool DeveloperMode::HasOption(pcstr option)
{
    if (!Core.Params || !option || !*option)
        return false;

    const size_t length = xr_strlen(option);
    for (pcstr current = Core.Params; (current = strstr(current, option)) != nullptr; ++current)
    {
        const bool beginsToken = current == Core.Params || current[-1] == ' ' || current[-1] == '\t';
        const bool endsToken = current[length] == '\0' || current[length] == ' ' || current[length] == '\t';
        if (beginsToken && endsToken)
            return true;
    }
    return false;
}

bool DeveloperMode::Requested()
{
    return HasOption("-dev_level");
}
