#include "stdafx.h"
#pragma hdrstop
#include "command_line_key.h"
#include "xrMemory.h"
#include "log.h"

template<> command_line_key<bool> *command_line_key<bool>::l_head = nullptr;
template<> command_line_key<int> *command_line_key<int>::l_head = nullptr;
template<> command_line_key<pcstr> *command_line_key<pcstr>::l_head = nullptr;

// command_line_key

// when adding make sure it's beginning with a '-'
template < typename T >
command_line_key<T>::command_line_key(pcstr opname, pcstr desc,T defval, bool req)
    : option_name(opname), description(desc), provided(false),
      required(req), argument(defval)
{
    // add the option list node
    l_next = l_head;
    l_head = this;
}

template < typename T >
command_line_key<T> *command_line_key<T>::find_option(pcstr flag_name)
{
    auto current_node =  command_line_key<T>::l_head;
    while (current_node != nullptr)
    {
        if (!xr_strcmp(current_node->option_name, flag_name))
            break;

        current_node = current_node->l_next;
    }
    return current_node;
}

template < typename T >
command_line_key<T>::~command_line_key()
{
    auto current_node =  command_line_key<T>::l_head;
    command_line_key<T> *prev_node = nullptr;
    while (current_node != nullptr)
    {
	    if (current_node == this)
            break;

        prev_node = current_node;
        current_node = current_node->l_next;
    }

    if (!current_node)
        return;

    if (prev_node)
        prev_node->l_next = current_node->l_next;
    else
        command_line_key<T>::l_head = current_node->l_next;
}

template < typename T >
bool command_line_key<T>::IsProvided() const
{
    return provided;
}

template < typename T >
T command_line_key<T>::OptionValue() const
{
    return argument;
}

template < typename T >
bool command_line_key<T>::CheckArguments()
{
    auto current_node =  command_line_key<T>::l_head;
    while (current_node != nullptr)
    {
        if (current_node->required && !current_node->provided)
        {
            Log("Error: Required option %s", current_node->option_name);
            return false;
        }
        current_node = current_node->l_next;
    }
    return true;
}

template < typename T >
void command_line_key<T>::PrintHelp()
{
    auto current_node = command_line_key<T>::l_head;
    while (current_node != nullptr)
    {
        pcstr isreq = current_node->required ? "(mandatory)" : "(optional)";
        Msg("%-20s \t %-10s \t %-25s", current_node->option_name,
            isreq, current_node->description);
        current_node = current_node->l_next;
    }
}

bool ParseCommandLine(int argc, char** argv)
{
    // put these back into class methods?
    for (int n = 1; n < argc; n++)
    {
        if (!IsOptionFlag(argv[n]))
        {
            Msg("Unknown option/argument <%s>", argv[n]);
            continue;
        }

        pcstr current_arg = argv[n];
        // this is for later
        // pcstr current_arg = &argv[n][1];

        // is this a bool option?
        if (auto clkey = command_line_key<bool>::find_option(current_arg))
        {
            clkey->argument = true;
            clkey->provided = true;
            continue;
        }
        // is this an int argument?
        else if (auto clkey = command_line_key<int>::find_option(current_arg))
        {
            if ((n + 1 >= argc) || IsOptionFlag(argv[n + 1]))
            {
                Msg("Error: Missing int argument for command line option <%s>",
                    current_arg);
                return false;
            }
            clkey->argument = std::stoi(argv[++n]);
            clkey->provided = true;
            continue;
        }
        // is this a string argument?
        else if (auto clkey = command_line_key<pcstr>::find_option(current_arg))
        {
            if ((n + 1 >= argc) || IsOptionFlag(argv[n + 1]))
            {
                Msg("Error: Missing string argument for command line option <%s>",
                    current_arg);
                return false;
            }
            clkey->argument = argv[++n];
            clkey->provided = true;
            continue;
        }
        Msg("Unknown option <%s>", argv[n]);
    }
    return true;
}


bool CLCheckAllArguments()
{
    return (command_line_key<bool>::CheckArguments()
            && command_line_key<int>::CheckArguments()
            && command_line_key<pcstr>::CheckArguments());
}

XRCORE_API void CLPrintAllHelp()
{
    command_line_key<bool>::PrintHelp();
    command_line_key<int>::PrintHelp();
    command_line_key<pcstr>::PrintHelp();
}

template class XRCORE_API command_line_key<bool>;
template class XRCORE_API command_line_key<int>;
template class XRCORE_API command_line_key<pcstr>;
