#pragma once

#include "xrCommon/xr_list.h"
#include "_std_extensions.h"

// This requires the keys be initialized as global variables
// flag name, option name, description must be static strings
// the flag name must be provided without a '-', it is
// implied when parsing the command line

template < typename T >
class XRCORE_API command_line_key
{
public:
    command_line_key(pcstr flag_name, pcstr desc,T defval, bool req = false);
    ~command_line_key();
    bool IsProvided() const;          // was the option provided in the CLI?
    T OptionValue() const;            // value provided with the option

    static bool CheckArguments();
    static void PrintHelp();
    friend XRCORE_API bool ParseCommandLine(int argc, char* argv[]);

private:
    pcstr option_name = nullptr;
    pcstr description = nullptr;
    bool required;
    bool provided = false;
    command_line_key<T> *l_next;

    T argument;

    static command_line_key<T> *find_option(pcstr flag_name);
    static command_line_key<T> *l_head;
};

inline static bool IsOptionFlag(pcstr buf)
{
    return (buf && buf[0] == '-');
}

XRCORE_API bool ParseCommandLine(int argc, char* argv[]);
XRCORE_API bool CLCheckAllArguments();
XRCORE_API void CLPrintAllHelp();
