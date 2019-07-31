#pragma once
#include "xrScriptEngine/xrScriptEngine.hpp"
#include "xrCore/xrstring.h"
#include "xrCommon/xr_list.h"

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
        pcstr m_id;
        xr_list<shared_str> m_deps;
        ExporterFunc m_exporterFunc;
        bool m_done;
        Node* m_prevNode;
        Node* m_nextNode;
        static Node* FirstNode;
        static Node* LastNode;
        static size_t NodeCount;

    public:
        Node(const char* id, std::initializer_list<shared_str> deps, ExporterFunc exporterFunc);
        ~Node();

        void Export(lua_State* luaState);
        void Reset() { m_done = false; }
        const char* GetId() const { return m_id; }
        size_t GetDependencyCount() const { return m_deps.size(); }
        const xr_list<shared_str>& GetDependencyIds() const { return m_deps; }
        Node* GetPrev() const { return m_prevNode; }
        Node* GetNext() const { return m_nextNode; }
        static Node* GetFirst() { return FirstNode; }
        static Node* GetLast() { return LastNode; }
        static size_t GetCount() { return NodeCount; }
    private:
        bool HasDependency(const Node* node) const;
        static void InsertAfter(Node* target, Node* node);
    };

    ScriptExporter() = delete;
    static void Export(lua_State* luaState);
    static void Reset();
};
}

#define SCRIPT_INHERIT_1(_1) #_1
#define SCRIPT_INHERIT_2(_1, _2) #_1, #_2
#define SCRIPT_INHERIT_3(_1, _2, _3) #_1, #_2, #_3
#define SCRIPT_INHERIT_4(_1, _2, _3, _4) #_1, #_2, #_3, #_4

#define EXPAND_MACRO(x) x
#define OVERLOAD_MACRO(_1, _2, _3, _4, count, ...) SCRIPT_INHERIT_##count
#define SCRIPT_INHERIT(...) EXPAND_MACRO(OVERLOAD_MACRO(__VA_ARGS__, 4, 3, 2, 1, 0)(__VA_ARGS__))

// clang-format off
#define SCRIPT_EXPORT_WRAP(id, dependencies, ...)                                       \
namespace XRay                                                                          \
{                                                                                       \
namespace ScriptExportDetails                                                           \
{                                                                                       \
__pragma(optimize("s", on))                                                             \
static void id##_ScriptExport(lua_State* luaState)                                      \
__VA_ARGS__                                                                             \
__pragma(warning(push)) __pragma(warning(disable : 4003))                               \
static const ScriptExporter::Node id##_ScriptExporterNode(                              \
    #id, {SCRIPT_INHERIT dependencies}, id##_ScriptExport);                             \
__pragma(warning(pop))                                                                  \
}                                                                                       \
}

/////////////////////////////////////////////////////////////////////////////

#define SCRIPT_EXPORT_FUNC_WRAP(id, dependencies, func)                                 \
namespace XRay                                                                          \
{                                                                                       \
namespace ScriptExportDetails                                                           \
{                                                                                       \
__pragma(optimize("s", on))                                                             \
static void id##_ScriptExport(lua_State* luaState) { func(luaState); }                  \
__pragma(warning(push)) __pragma(warning(disable : 4003))                               \
static const ScriptExporter::Node id##_ScriptExporterNode(                              \
    #id, {SCRIPT_INHERIT dependencies}, id##_ScriptExport);                             \
__pragma(warning(pop))                                                                  \
}                                                                                       \
}
// clang-format on

// register script exporter (accepts function body)
#define SCRIPT_EXPORT(id, dependencies, ...) SCRIPT_EXPORT_WRAP(id, dependencies, __VA_ARGS__)
// register script exporter (accepts function)
#define SCRIPT_EXPORT_FUNC(id, dependencies, func) SCRIPT_EXPORT_FUNC_WRAP(id, dependencies, func)
