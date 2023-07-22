#include "pch.hpp"
#include "xrScriptEngine.hpp"

size_t luabind_it_distance(luabind::iterator first, const luabind::iterator& last)
{
    size_t result = 0;
    while (first != last)
    {
        ++first;
        ++result;
    }
    return result;
}
