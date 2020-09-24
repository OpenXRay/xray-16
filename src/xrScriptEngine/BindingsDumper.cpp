#include "pch.hpp"
#include "BindingsDumper.hpp"
#include <regex>

int BindingsDumper::GetIdentSize() const { return options.ShiftWidth * shiftLevel; }
void BindingsDumper::Print(const char* s) { writer->w(s, xr_strlen(s)); }
void BindingsDumper::Print(const char* s, int len) { writer->w(s, len); }
void BindingsDumper::Printf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    writer->VPrintf(format, args);
    va_end(args);
}

void BindingsDumper::PrintIndented(const char* s)
{
    writer->w_printf(
        "%*s"
        "%s",
        GetIdentSize(), "", s);
}

void BindingsDumper::PrintfIndented(const char* format, ...)
{
    writer->w_printf("%*s", GetIdentSize(), "");
    va_list args;
    va_start(args, format);
    writer->VPrintf(format, args);
    va_end(args);
}

namespace
{
luabind::detail::function_object* get_upvalue_function(lua_State* ls, int n)
{
    using namespace luabind::detail;
    function_object* f = nullptr;
    if (lua_getupvalue(ls, -1, n))
    {
        int ltype = lua_type(ls, -1);
        if (ltype == LUA_TFUNCTION)
        {
            if (lua_getupvalue(ls, -1, 1))
            {
                if (lua_type(ls, -1) == LUA_TUSERDATA)
                    f = *static_cast<function_object**>(lua_touserdata(ls, -1));
                lua_pop(ls, 1); // upvalue
            }
        }
        lua_pop(ls, 1); // upvalue
    }
    return f;
}

void ReplaceArg(lua_State* ls, int argIndex, const char* subst)
{
    lua_pushstring(ls, subst);
    lua_replace(ls, argIndex - 1);
}

int StripArg(lua_State* ls, int argIndex, bool commaOnly = true)
{
    int removed = 1;
    lua_remove(ls, argIndex);
    const char* comma = lua_tostring(ls, argIndex + 1);
    if (!commaOnly || !xr_strcmp(comma, ","))
    {
        lua_remove(ls, argIndex + 1);
        removed++;
    }
    return removed;
}
}

void BindingsDumper::PrintFunction(SignatureFormatter formatter, const void* fcontext /*= nullptr*/)
{
    using namespace luabind::detail;
    bool done = false;
    int cfunc = lua_iscfunction(ls, -1);
    int luabindFunc = is_luabind_function(ls, -1);
    bool hasupvalue = lua_getupvalue(ls, -1, 1) != nullptr;
    if (hasupvalue)
    {
        if (luabindFunc)
        {
            if (lua_type(ls, -1) == LUA_TUSERDATA)
            {
                auto fobj = *static_cast<function_object**>(lua_touserdata(ls, -1));
                if (formatter)
                {
                    SignatureFormatterParams params;
                    params.Function = fobj;
                    params.Context = fcontext;
                    (this->*formatter)(params);
                }
                else
                {
                    int signatureLen = fobj->format_signature(ls, fobj->name.c_str());
                    auto signature = lua_tostring(ls, -1);
                    PrintfIndented("%s;\n", signature);
                    lua_pop(ls, signatureLen);
                }
                done = true;
            }
        }
        lua_pop(ls, 1); // pop upvalue
    }
    if (cfunc && !done && hasupvalue) // property
    {
        const char* propName = lua_tostring(ls, -2);
        function_object* getter = get_upvalue_function(ls, 1);
        function_object* setter = get_upvalue_function(ls, 2);
        R_ASSERT(getter);
        int signatureLen = getter->format_signature(ls, "#");
        const char* signature = lua_tostring(ls, -1);
        int typeLen = std::strchr(signature, '#') - signature - 1;
        PrintIndented("");
        Print(signature, typeLen);
        Printf(" %s { get;", propName);
        if (setter)
            Print(" set;");
        Print(" }\n");
        lua_pop(ls, signatureLen);
    }
}

void BindingsDumper::FormatStaticFunction(const SignatureFormatterParams& params)
{
    using namespace luabind::detail;
    function_object* fobj = params.Function;
    int signatureLen = fobj->format_signature(ls, fobj->name.c_str());
    const char* signature = lua_tostring(ls, -1);
    PrintfIndented("static %s;\n", signature);
    lua_pop(ls, signatureLen);
};

void BindingsDumper::PrintStaticFunction() { PrintFunction(&BindingsDumper::FormatStaticFunction); }
void BindingsDumper::FormatMemberFunction(const SignatureFormatterParams& params)
{
    auto refClassName = static_cast<const char*>(params.Context);
    bool stripReturnValue = false; // for constructors and operators
    xr_string funcName;
    auto refFuncName = params.Function->name;
    if (refFuncName == "__init")
    {
        funcName = refClassName;
        stripReturnValue = true;
    }
    else // check operators
    {
        auto it = operatorSubst.find(refFuncName);
        if (it != operatorSubst.end())
        {
            funcName = it->second;
            stripReturnValue = true;
        }
        else
            funcName = refFuncName;
    }
    bool concat = !(options.IgnoreDerived || options.StripThis || stripReturnValue);
    int signLen = params.Function->format_signature(ls, funcName.c_str(), concat);
    if (!concat)
    {
        int argCount = (signLen - 4) / 2;
        R_ASSERT(argCount > 0);
        //      -n+0   -n+1  -n+2    -n+3 -n+4              -1
        // [return_type][ ][func_name][(][arg1][,][arg2]...[)]
        int offset = 0;
        if (stripReturnValue)
        {
            offset = StripArg(ls, -signLen, false);
            signLen -= offset;
        }
        int argIndex = -signLen + 4 - offset;
        xr_string arg = lua_tostring(ls, argIndex);
        xr_string className = refClassName;
        // check if arg matches 'className[ const]{*|&}'
        std::regex matcher(className + "( const)?(\\*|&)$");
        if (std::regex_match(arg, matcher)) // non-derived member function
        {
            if (options.StripThis)
                signLen -= StripArg(ls, argIndex);
        }
        else
        {
            // check special cases: opertators and constructors
            // void __tostring(lua_State*, ClassName&); // operator
            // void __init(luabind::argument const&, int); // constructor
            if (arg == "lua_State*" && argCount > 1) // operator?
            {
                // 1] check next argument:
                int nextArgIndex = argIndex + 2;
                const char* nextArg = lua_tostring(ls, nextArgIndex);
                if (className.append("&") != nextArg) // derived?
                {
                    // if next!=className && ignoreDerived => ignore
                    if (options.IgnoreDerived)
                    {
                        lua_pop(ls, signLen);
                        return;
                    }
                }
                // 2] remove arg+comma
                signLen -= StripArg(ls, argIndex);
                // 3] if stripThis => remove next
                argIndex = nextArgIndex;
            }
            else if (arg == "luabind::argument const&") // constructor?
            {
                if (!options.StripThis)
                    ReplaceArg(ls, argIndex, className.append("&").c_str());
            }
            else if (options.IgnoreDerived) // some derived function, ignore?
            {
                lua_pop(ls, signLen);
                return;
            }
            if (options.StripThis)
                signLen -= StripArg(ls, argIndex);
        }
        lua_concat(ls, signLen);
    }
    const char* signature = lua_tostring(ls, -1);
    PrintfIndented("%s;\n", signature);
    lua_pop(ls, 1); // pop concatenated signature
};

void BindingsDumper::PrintMemberFunction(const char* className)
{
    PrintFunction(&BindingsDumper::FormatMemberFunction, className);
}

void BindingsDumper::PrintFunction() { PrintFunction(nullptr); }
void BindingsDumper::PrintIntConstant(const char* name, int value)
{
    PrintfIndented("const int %s = %d;\n", name, value);
}

void BindingsDumper::PrintClass()
{
    using namespace luabind;
    using namespace luabind::detail;
    auto crep = static_cast<class_rep*>(lua_touserdata(ls, -1));
    bool cppClass = crep->get_class_type() == class_rep::cpp_class;
    PrintIndented(cppClass ? "[cpp]\n" : "[lua]\n");
    PrintfIndented("class %s", crep->name());
    const auto& bases = crep->bases();
    size_t baseCount = bases.size();
    if (baseCount)
        Print(" : ");
    for (size_t i = 0; i < baseCount; i++)
    {
        if (i)
            Print(", ");
        const char* baseName = bases[i].base->name();
        if (!*baseName)
            baseName = "<unknown>";
        Print(baseName);
    }
    Print("\n");
    PrintIndented("{\n");
    shiftLevel++;
    // print static members (static functions + nested classes)
    crep->get_default_table(ls);
    object staticMembers(from_stack(ls, -1));
    for (luabind::iterator it(staticMembers), end; it != end; ++it)
    {
        auto proxy = *it;
        int prev = lua_gettop(ls);
        proxy.push(ls);
        if (is_class_rep(ls, -1))
            PrintClass();
        else if (is_luabind_function(ls, -1, false))
            PrintStaticFunction();
        lua_pop(ls, 1);
        R_ASSERT(lua_gettop(ls) == prev);
    }
    lua_pop(ls, 1); // pop default table
    // print constants
    auto& constants = crep->static_constants();
    for (auto& c : constants)
        PrintIntConstant(c.first, c.second);
    // print functions and properties
    crep->get_table(ls);
    object members(from_stack(ls, -1));
    for (luabind::iterator it(members), end; it != end; ++it)
    {
        auto proxy = *it;
        int prev = lua_gettop(ls);
        proxy.push(ls);
        int ltype = luabind::type(proxy);
        if (ltype == LUA_TFUNCTION) // XXX: print class members in reverse order
            PrintMemberFunction(crep->name());
        lua_pop(ls, 1);
        R_ASSERT(lua_gettop(ls) == prev);
    }
    lua_pop(ls, 1); // pop table
    shiftLevel--;
    PrintIndented("}\n");
}

void BindingsDumper::PrintNamespace(luabind::object& namesp)
{
    using namespace luabind;
    using namespace luabind::detail;
    int scopeFunctions = 0, scopeClasses = 0, scopeNamespaces = 0;
    for (luabind::iterator it(namesp), end; it != end; ++it)
    {
        auto proxy = *it;
        int ltype = luabind::type(proxy);
        switch (ltype)
        {
        case LUA_TFUNCTION: // free function
            scopeFunctions++;
            functions.push(it);
            break;
        case LUA_TUSERDATA: // class
            scopeClasses++;
            classes.push(it);
            break;
        case LUA_TTABLE: // namespace
            scopeNamespaces++;
            namespaces.push(it);
            break;
        default: PrintfIndented("[?] ltype = %s\n", lua_typename(ls, ltype)); break;
        }
    }
    for (int i = 0; i < scopeFunctions; i++)
    {
        auto proxy = *functions.top();
        functions.pop();
        proxy.push(ls);
        PrintFunction();
        lua_pop(ls, 1);
    }
    for (int i = 0; i < scopeClasses; i++)
    {
        auto proxy = *classes.top();
        classes.pop();
        proxy.push(ls);
        if (is_class_rep(ls, -1))
            PrintClass();
        lua_pop(ls, 1);
    }
    for (int i = 0; i < scopeNamespaces; i++)
    {
        auto proxy = *namespaces.top();
        namespaces.pop();
        proxy.push(ls);
        object innerNamesp(from_stack(ls, -1));
        auto namespaceName = lua_tostring(ls, -2);
        PrintfIndented("namespace %s\n", namespaceName);
        PrintIndented("{\n");
        shiftLevel++;
        PrintNamespace(innerNamesp);
        shiftLevel--;
        PrintIndented("}\n");
        lua_pop(ls, 1);
    }
}

BindingsDumper::BindingsDumper()
{
    std::pair<const char*, const char*> subst[] = {{"__add", "operator+"}, {"__sub", "operator-"},
        {"__mul", "operator*"}, {"__div", "operator/"}, {"__pow", "operator^"}, {"__lt", "operator<"},
        {"__le", "operator<="}, {"__gt", "operator>"}, {"__ge", "operator>="}, {"__eq", "operator=="},
        {"__tostring", "operator string"}};
    const u32 substCount = sizeof(subst) / sizeof(*subst);
    for (u32 i = 0; i < substCount; i++)
        operatorSubst.insert(subst[i]);
}

void BindingsDumper::Dump(lua_State* luaState, IWriter* outStream, const Options& opt)
{
    ls = luaState;
    options = opt;
    shiftLevel = 0;
    writer = outStream;
    luabind::set_custom_type_marking(false);
    luabind::object globals = luabind::globals(ls);
    PrintNamespace(globals);
    luabind::set_custom_type_marking(true);
}
