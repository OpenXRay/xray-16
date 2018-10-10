#include "StdAfx.h"

namespace ai_dbg
{
struct var
{
    enum type_t
    {
        string,
        real
    };

    type_t type;
    char str[1024];
    float fval;
};

typedef xr_map<xr_string, var> debug_vars_t;
static debug_vars_t s_debug_vars;

void set_var(const char* name, float value)
{
    var new_var;
    new_var.fval = value;
    new_var.type = var::real;

    debug_vars_t::iterator it = s_debug_vars.find(xr_string(name));
    if (it != s_debug_vars.end())
    {
        s_debug_vars.erase(it);
    }

    s_debug_vars.insert(make_pair(xr_string(name), new_var));
}

bool get_var(const char* name, float& value)
{
    debug_vars_t::iterator it = s_debug_vars.find(xr_string(name));
    if (it == s_debug_vars.end())
    {
        return false;
    }

    var& v = (*it).second;
    if (v.type != var::real)
    {
        return false;
    }

    value = v.fval;
    return true;
}

bool get_var(const char* name, u32& value)
{
    float fval;
    bool res = get_var(name, fval);
    if (res)
    {
        value = (u32)fval;
    }
    return res;
}

bool get_var(const char* name, bool& value)
{
    float fval;
    bool res = get_var(name, fval);
    if (res)
    {
        value = fval != 0;
    }
    return res;
}

void show_var(const char* name)
{
    debug_vars_t::iterator it = s_debug_vars.find(xr_string(name));
    if (it == s_debug_vars.end())
    {
        return;
    }

    var& v = (*it).second;

    if (v.type == var::real)
    {
        Msg("%s = %f", (*it).first.c_str(), v.fval);
    }
    else if (v.type == var::string)
    {
        Msg("%s = \"%s\"", (*it).first.c_str(), v.str);
    }
}

} // namespace ai_dbg
