#pragma once

#include "xrCommon/xr_list.h"
#include "_std_extensions.h"

template<typename T> class XRCORE_API command_line_key;

template<typename T>
class XRCORE_API command_line_key
{
public:
    command_line_key(pcstr flag_name, pcstr desc, const T defval, bool req = false);
    ~command_line_key();
    bool IsProvided() const;          // was the option provided in the CLI?
    T OptionValue() const;            // value provided with the option

    static bool CheckArguments();
    static void PrintHelp();
    friend bool ParseCommandLine(int argc, char *argv[]);

private:
    pstr option_name = nullptr;
    pstr description = nullptr;
    bool required;
    bool provided = false;
    command_line_key<T> *l_next;

    T argument;

    void copy_argument(T arg);
    void free_argument();
    static command_line_key<T> *find_option(pcstr flag_name);
    static bool parse_option(pcstr option, pcstr arg);
    static command_line_key<T> *l_head;
};

inline static bool IsOptionFlag(pcstr buf)
{
    return (buf && buf[0] == '-');
}

XRCORE_API bool ParseCommandLine(int argc, char *argv[]);
XRCORE_API bool CLCheckAllArguments();
XRCORE_API void CLPrintAllHelp();
