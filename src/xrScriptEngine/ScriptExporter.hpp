#pragma once
#include "xrScriptEngine.hpp"

struct lua_State;

namespace XRay
{
class XRSCRIPTENGINE_API ScriptExporter
{
public:
    class XRSCRIPTENGINE_API Node
    {
    public:
        using ExporterFunc = void(__cdecl*)(lua_State* luaState);

    private:
        const char* id;
        size_t depCount;
        const char* const* deps;
        ExporterFunc exporterFunc;
        bool done;
        Node* prevNode;
        Node* nextNode;
        static Node* firstNode;
        static Node* lastNode;
        static size_t nodeCount;

    public:
        Node(const char* id, size_t depCount, const char* const* deps, ExporterFunc exporterFunc);

        void Export(lua_State* luaState);
        void Reset() { done = false; }
        const char* GetId() const { return id; }
        size_t GetDependencyCount() const { return depCount; }
        const char* const* GetDependencyIds() const { return deps; }
        Node* GetPrev() const { return prevNode; }
        Node* GetNext() const { return nextNode; }
        static Node* GetFirst() { return firstNode; }
        static Node* GetLast() { return lastNode; }
        static size_t GetCount() { return nodeCount; }
    private:
        bool HasDependency(const Node* node) const;
        static void InsertAfter(Node* target, Node* node);
    };

    ScriptExporter() = delete;
    static void Export(lua_State* luaState);
    static void Reset();
};
}

#define NOOP(...) __VA_ARGS__
#define GLUE(x, y) x y
#define ARG_COUNT(_1, _2, _3, _4, count, ...) count
#define EXPAND_ARGS(args) ARG_COUNT args
#define ARG_COUNT_4(...) EXPAND_ARGS((__VA_ARGS__, 4, 3, 2, 1, 0))
#define OVERLOAD_MACRO2(name, count) name##count
#define OVERLOAD_MACRO1(name, count) OVERLOAD_MACRO2(name, count)
#define OVERLOAD_MACRO(name, count) OVERLOAD_MACRO1(name, count)
#define CALL_OVERLOAD(name, ...) GLUE(OVERLOAD_MACRO(name, ARG_COUNT_4(__VA_ARGS__)), (__VA_ARGS__))

#define SCRIPT_INHERIT1(_1) #_1
#define SCRIPT_INHERIT2(_1, _2) #_1, #_2
#define SCRIPT_INHERIT3(_1, _2, _3) #_1, #_2, #_3
#define SCRIPT_INHERIT4(_1, _2, _3, _4) #_1, #_2, #_3, #_4
#define SCRIPT_INHERIT(...) CALL_OVERLOAD(SCRIPT_INHERIT, __VA_ARGS__)

#define SCRIPT_EXPORT_WRAP(id, dependencies, ...)                                                                 \
    \
namespace XRay                                                                                                    \
    \
{                                                                                                          \
        \
namespace ScriptExportDetails                                                                                     \
        \
{                                                                                                      \
            __pragma(warning(push)) __pragma(warning(disable : 4003)) static const char* const id##_Deps[] = {    \
                nullptr, SCRIPT_INHERIT(NOOP dependencies)};                                                      \
            __pragma(warning(pop)) __pragma(optimize("s", on)) static void id##_ScriptExport(lua_State* luaState) \
                __VA_ARGS__ static const ScriptExporter::Node id##_ScriptExporterNode(                            \
                    #id, sizeof(id##_Deps) / sizeof(*id##_Deps) - 1, id##_Deps + 1, id##_ScriptExport);           \
        \
}                                                                                                      \
    \
}

#define SCRIPT_EXPORT_FUNC_WRAP(id, dependencies, func)                                                           \
    \
namespace XRay                                                                                                    \
    \
{                                                                                                          \
        \
namespace ScriptExportDetails                                                                                     \
        \
{                                                                                                      \
            __pragma(warning(push)) __pragma(warning(disable : 4003)) static const char* const id##_Deps[] = {    \
                nullptr, SCRIPT_INHERIT(NOOP dependencies)};                                                      \
            __pragma(warning(pop)) __pragma(optimize("s", on)) static void id##_ScriptExport(lua_State* luaState) \
            {                                                                                                     \
                func(luaState);                                                                                   \
            }                                                                                                     \
            static const ScriptExporter::Node id##_ScriptExporterNode(                                            \
                #id, sizeof(id##_Deps) / sizeof(*id##_Deps) - 1, id##_Deps + 1, id##_ScriptExport);               \
        \
}                                                                                                      \
    \
}

// register script exporter (accepts function body)
#define SCRIPT_EXPORT(id, dependencies, ...) SCRIPT_EXPORT_WRAP(id, dependencies, __VA_ARGS__)
// register script exporter (accepts function)
#define SCRIPT_EXPORT_FUNC(id, dependencies, func) SCRIPT_EXPORT_FUNC_WRAP(id, dependencies, func)
