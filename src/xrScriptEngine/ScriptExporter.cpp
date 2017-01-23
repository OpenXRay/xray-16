#include "pch.hpp"
#include "ScriptEngineConfig.hpp"
#include "ScriptExporter.hpp"
#include "xrCore/xrCore.h"

using namespace XRay;

ScriptExporter::Node *ScriptExporter::Node::firstNode = nullptr;
ScriptExporter::Node *ScriptExporter::Node::lastNode = nullptr;
size_t ScriptExporter::Node::nodeCount = 0;

ScriptExporter::Node::Node(const char *id, size_t depCount, const char *const *deps, ExporterFunc exporterFunc)
{            
    this->id = id;
    this->depCount = depCount;
    this->deps = deps;
    this->exporterFunc = exporterFunc;
    done = false;
    InsertAfter(nullptr, this);
}

void ScriptExporter::Node::Export(lua_State *luaState)
{
    if (done)
    {
#ifdef CONFIG_SCRIPT_ENGINE_LOG_SKIPPED_EXPORTS
        Msg("* ScriptExporter: skipping exported node %s", id);
#endif
        return;
    }
    // export dependencies recursively
    for (size_t i = 0; i<depCount; i++)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node *n = GetFirst(); n; n = n->GetNext())
        {
            if (!n->done && !strcmp(deps[i], n->id))
            {
                n->Export(luaState);
                break;
            }
        }
    }
#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: exporting node %s", id);
#endif
    exporterFunc(luaState);
    done = true;
}

bool ScriptExporter::Node::HasDependency(const Node *node) const
{
    for (size_t i = 0; i<depCount; i++)
    {
        if (!strcmp(deps[i], node->id))
            return true;
    }
    for (size_t i = 0; i<depCount; i++)
    {
        // check if 'deps[i]' depends on 'node'
        for (Node *n = GetFirst(); n; n = n->GetNext())
        {
            if (!strcmp(deps[i], n->id))
            {
                if (n->HasDependency(node))
                    return true;
                break;
            }
        }
    }
    return false;
}

void ScriptExporter::Node::InsertAfter(Node *target, Node *node)
{
    if (!target)
    {
        node->prevNode = nullptr;
        node->nextNode = firstNode;
        if (firstNode)
            firstNode->prevNode = node;
        else
            lastNode = node;
        firstNode = node;
    }
    else
    {
        node->prevNode = target;
        node->nextNode = target->nextNode;
        if (target==lastNode)
            lastNode = node;
        target->nextNode = node;
    }
    nodeCount++;
}

void ScriptExporter::Export(lua_State *luaState)
{
#ifdef CONFIG_SCRIPT_ENGINE_LOG_EXPORTS
    Msg("* ScriptExporter: total nodes: %zu", Node::GetCount());
    for (auto node = Node::GetFirst(); node; node = node->GetNext())
    {
        Msg("* %s", node->GetId());
        size_t depCount = node->GetDependencyCount();
        const char *const *depIds = node->GetDependencyIds();
        for (int i = 0; i<depCount; i++)
            Msg("* <- %s", depIds[i]);
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
