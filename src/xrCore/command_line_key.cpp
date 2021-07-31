#include "command_line_key.h"
#include "xrMemory.h"
#include "log.h"

template<> command_line_key<bool> *command_line_key<bool>::l_head = nullptr;
template<> command_line_key<int> *command_line_key<int>::l_head = nullptr;
template<> command_line_key<pcstr> *command_line_key<pcstr>::l_head = nullptr;

// command_line_key

// when adding make sure it's beginning with a '-'
template<typename T>
command_line_key<T>::command_line_key(pcstr opname, pcstr desc,T defval, bool req)
    : provided(false), required(req)
{
    if(!IsOptionFlag(opname))
    {
        size_t opsz = xr_strlen(opname) + 1;
        option_name = xr_alloc<char>(opsz);
        option_name[0] = '-';
        xr_strcpy(&option_name[1], opsz - 1, opname);
    }
    else
    {
        option_name = xr_strdup(opname);
    }
    description = xr_strdup(desc);
    copy_argument(defval);

    // add the option list node
    l_next = l_head;
    l_head = this;
}


template<typename T>
void command_line_key<T>::copy_argument(T arg) {
    argument = arg;
}

template<>
void command_line_key<pcstr>::copy_argument(pcstr arg) {
    argument = xr_strdup(arg);
}

template<typename T>
void command_line_key<T>::free_argument() {
    return;
}

template<>
void command_line_key<pcstr>::free_argument() {
    xr_free(argument);
}


template<typename T>
command_line_key<T> *command_line_key<T>::find_option(pcstr flag_name)
{
    auto current_node =  command_line_key<T>::l_head;
    while(current_node != nullptr)
    {
        if (!xr_strcmp(current_node->option_name, flag_name)) break;
        current_node = current_node->l_next;
    }
    return current_node;
}

template<typename T>
command_line_key<T>::~command_line_key()
{
    xr_free(option_name);
    xr_free(description);
    free_argument();

    command_line_key<T> *current_node =  command_line_key<T>::l_head;
    command_line_key<T> *prev_node = nullptr;
    while(current_node != nullptr)
    {
        if (current_node == this) break;
        prev_node = current_node;
        current_node = current_node->l_next;
    }

    if(!current_node)
        return;

    if(prev_node)
        prev_node->l_next = current_node->l_next;
    else
        command_line_key<T>::l_head = current_node->l_next;
}

template<typename T>
bool command_line_key<T>::IsProvided() const
{
    return provided;
}

template<typename T>
T command_line_key<T>::OptionValue() const
{
    return argument;
}

template<typename T>
bool command_line_key<T>::CheckArguments()
{
    auto current_node =  command_line_key<T>::l_head;
    while(current_node != nullptr)
    {
        if(current_node->required && !current_node->provided)
        {
            Log("Error: Required option %s", current_node->option_name);
            return false;
        }
        current_node = current_node->l_next;
    }
    return true;
}

template<>
bool command_line_key<bool>::parse_option(pcstr option, pcstr arg)
{
    auto clkey = command_line_key<bool>::find_option(option);
    if(!clkey) return false; // not found

    clkey->argument = true;
    clkey->provided = true;
    return true;                // found and set
}

template<>
bool command_line_key<int>::parse_option(pcstr option, pcstr arg)
{
    auto clkey = command_line_key<int>::find_option(option);
    if(!clkey) return false; // not found

    clkey->argument = std::stoi(arg); // may throw
    clkey->provided = true;
    return true;
}

template<>
bool command_line_key<pcstr>::parse_option(pcstr option, pcstr arg)
{
    auto clkey = command_line_key<pcstr>::find_option(option);
    if(!clkey) return false; // not found

    clkey->argument = xr_strdup(arg);
    clkey->provided = true;
    return true;
}

template<typename T>
void command_line_key<T>::PrintHelp() {
    auto current_node = command_line_key<T>::l_head;
    while(current_node != nullptr)
    {
        pcstr isreq = current_node->required ? "(mandatory)" : "(optional)";
        Msg("%-10s \t %-10s \t %-25s", current_node->option_name,
            isreq, current_node->description);
    }
}

bool ParseCommandLine(int argc, char **argv)
{
    // put these back into class methods?
    for(int n = 1; n < argc; n++)
    {
        if(!IsOptionFlag(argv[n]))
        {
            Msg("Unknown option/argument <%s>", argv[n]);
            continue;
        }

        // is this a bool option?
        if(command_line_key<bool>::parse_option(argv[n], nullptr))
        {
            continue;
        }
        // the rest of the flags will require an argument
        else if((n + 1 >= argc) || IsOptionFlag(argv[n + 1]))
        {
            Msg("Error: Missing argument for command line option <%s>", argv[n]);
            return false;
        }
        else if(command_line_key<int>::parse_option(argv[n], argv[n + 1])
                || command_line_key<pcstr>::parse_option(argv[n], argv[n + 1]))
        {
            n++;
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

XRCORE_API void CLPrintAllHelp() {
    command_line_key<bool>::PrintHelp();
    command_line_key<int>::PrintHelp();
    command_line_key<pcstr>::PrintHelp();
}

template class XRCORE_API command_line_key<bool>;
template class XRCORE_API command_line_key<int>;
template class XRCORE_API command_line_key<pcstr>;
