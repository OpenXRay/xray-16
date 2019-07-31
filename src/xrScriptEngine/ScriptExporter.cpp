#include "pch.hpp"
#include "ScriptEngineConfig.hpp"
#include "ScriptExporter.hpp"
#include "xrCore/xrCore.h"

using namespace XRay;

ScriptExporter::Node* ScriptExporter::Node::FirstNode = nullptr;
ScriptExporter::Node* ScriptExporter::Node::LastNode = nullptr;
size_t ScriptExporter::Node::NodeCount = 0;

ScriptExporter::Node::Node(const char* id, std::initializer_list<shared_str> deps, ExporterFunc exporterFunc)
: m_deps(deps)
{
    m_id = id;
    m_exporterFunc = exporterFunc;
    m_done = false;
    InsertAfter(nullptr, this);
}

ScriptExporter::Node::~Node()
{
    // Remap locals
    // ... <-> N <-> this <-> N <-> ...
    {
        if (m_prevNode)
            m_prevNode->m_nextNode = m_nextNode;

        if (m_nextNode)
            m_nextNode->m_prevNode = m_prevNode;
    }

    // Remap globals
    {
        // this <-> N <-> ...
        if (FirstNode == this)
            FirstNode = m_nextNode;

        // ... <-> N <-> this
        if (LastNode == this)
            LastNode = m_prevNode;
    }
}

void ScriptExporter::Node::Export(lua_State* luaState)
{
    if (m_done)
    {
#ifdef CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS
        Msg("* ScriptExporter: skipping exported node %s", id);
#endif
        return;
    }
    // export dependencies recursively
    for (const shared_str& it : m_deps)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node* n = GetFirst(); n; n = n->GetNext())
        {
            if (!n->m_done && !xr_strcmp(it, n->m_id))
            {
                n->Export(luaState);
                break;
            }
        }
    }
#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: exporting node %s", id);
#endif
    m_exporterFunc(luaState);
    m_done = true;
}

bool ScriptExporter::Node::HasDependency(const Node* node) const
{
    for (const shared_str& it : m_deps)
    {
        if (!xr_strcmp(it, node->m_id))
            return true;
    }
    for (const shared_str& it : m_deps)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node* n = GetFirst(); n; n = n->GetNext())
        {
            if (!xr_strcmp(it, n->m_id))
            {
                if (n->HasDependency(node))
                    return true;
                break;
            }
        }
    }
    return false;
}

void ScriptExporter::Node::InsertAfter(Node* target, Node* node)
{
    if (!target)
    {
        node->m_prevNode = LastNode;
        node->m_nextNode = nullptr;
        if (LastNode)
            LastNode->m_nextNode = node;
        else
            FirstNode = node;
        LastNode = node;
    }
    else
    {
        node->m_prevNode = target;
        node->m_nextNode = target->m_nextNode;
        if (target == LastNode)
            LastNode = node;
        target->m_nextNode = node;
    }
    NodeCount++;
}

void ScriptExporter::Export(lua_State* luaState)
{
#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: total nodes: %zu", Node::GetCount());
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
    {
        Msg("* %s", node->GetId());
        const xr_list<shared_str>& depIds = node->GetDependencyIds();
        for (const shared_str& it : depIds)
            Msg("* <- %s", it.c_str());
    }
#endif
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
        node->Export(luaState);
}

void ScriptExporter::Reset()
{
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
        node->Reset();
}
