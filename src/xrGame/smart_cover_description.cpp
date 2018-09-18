////////////////////////////////////////////////////////////////////////////
//	Module 		: smart_cover_description.cpp
//	Created 	: 16.08.2007
//	Author		: Alexander Dudin
//	Description : Smart cover description class
////////////////////////////////////////////////////////////////////////////

#include "pch_script.h"
#include "smart_cover_description.h"
#include "ai_space.h"
#include "xrScriptEngine/script_engine.hpp"
#include "Common/object_broker.h"
#include "smart_cover_loophole.h"
#include "smart_cover_object.h"
#include "ai_monster_space.h"
#include "smart_cover_transition.hpp"

using namespace MonsterSpace;
using smart_cover::description;
using smart_cover::loophole;
using smart_cover::detail::parse_float;
using smart_cover::detail::parse_string;
using smart_cover::detail::parse_table;
using smart_cover::detail::parse_fvector;
using smart_cover::detail::parse_int;

namespace smart_cover
{
static LPCSTR s_enter_loophole_id = "<__ENTER__>";
static LPCSTR s_exit_loophole_id = "<__EXIT__>";

shared_str transform_vertex(shared_str const& vertex_id, bool const& in)
{
    if (*vertex_id.c_str())
        return (vertex_id);

    if (in)
        return (s_enter_loophole_id);

    return (s_exit_loophole_id);
}

shared_str parse_vertex(luabind::object const& table, LPCSTR identifier, bool const& in)
{
    return (transform_vertex(parse_string(table, identifier), in));
}
} // namespace smart_cover

class id_predicate
{
    loophole const* m_loophole;

public:
    IC id_predicate(loophole const& loophole) : m_loophole(&loophole) {}
    IC bool operator()(loophole* const& loophole) const
    {
        VERIFY(loophole);
        return (m_loophole->id()._get() == loophole->id()._get());
    }
};

class enterable_predicate
{
public:
    IC bool operator()(loophole* const& loophole) const
    {
        VERIFY(loophole);
        return (loophole->enterable());
    }
};

class exitable_predicate
{
public:
    IC bool operator()(loophole* const& loophole) const
    {
        VERIFY(loophole);
        return (loophole->exitable());
    }
};

class usable_predicate
{
public:
    IC bool operator()(loophole* const& loophole) const
    {
        VERIFY(loophole);
        return (loophole->usable());
    }
};

description::description(shared_str const& table_id)
{
    load_loopholes(table_id);
    load_transitions(table_id);
    process_loopholes();
}

void description::load_loopholes(shared_str const& table_id)
{
    string256 temp;
    xr_strcpy(temp, "smart_covers.descriptions.");
    xr_strcat(temp, *table_id);
    xr_strcat(temp, ".loopholes");
    m_table_id = table_id;

    luabind::object loopholes;
    bool result = GEnv.ScriptEngine->function_object(temp, loopholes, LUA_TTABLE);
    VERIFY2(result, make_string("bad or missing loopholes table in smart_cover [%s]", table_id.c_str()));
    for (luabind::iterator I(loopholes), E; I != E; ++I)
    {
        luabind::object table = *I;
        if (luabind::type(table) != LUA_TTABLE)
        {
            VERIFY(luabind::type(table) != LUA_TNIL);
            continue;
        }

        smart_cover::loophole* loophole = new smart_cover::loophole(table);
        VERIFY(std::find_if(m_loopholes.begin(), m_loopholes.end(), id_predicate(*loophole)) == m_loopholes.end());
        m_loopholes.push_back(loophole);
    }

    VERIFY2(!m_loopholes.empty(), make_string("smart_cover [%s] doesn't have loopholes", m_table_id.c_str()));
    VERIFY2(std::find_if(m_loopholes.begin(), m_loopholes.end(), usable_predicate()) != m_loopholes.end(),
        make_string("smart_cover [%s] doesn't have usable loopholes", m_table_id.c_str()));
}

void description::process_loopholes()
{
    Loopholes::iterator I = m_loopholes.begin();
    Loopholes::iterator E = m_loopholes.end();

    for (; I != E; ++I)
    {
        ::loophole* current = *I;
        current->enterable(m_transitions.edge(transform_vertex("", true), current->id()) != 0);
        current->exitable(m_transitions.edge(current->id(), transform_vertex("", false)) != 0);
    }

    VERIFY2(std::find_if(m_loopholes.begin(), m_loopholes.end(), enterable_predicate()) != m_loopholes.end(),
        make_string("smart_cover [%s] doesn't have enterable loopholes", m_table_id.c_str()));

    VERIFY2(std::find_if(m_loopholes.begin(), m_loopholes.end(), exitable_predicate()) != m_loopholes.end(),
        make_string("smart_cover [%s] doesn't have exitable loopholes", m_table_id.c_str()));
}

void description::load_transitions(shared_str const& table_id)
{
    string256 temp;
    xr_strcpy(temp, "smart_covers.descriptions.");
    xr_strcat(temp, *table_id);
    xr_strcat(temp, ".transitions");

    luabind::object transitions;
    bool result = GEnv.ScriptEngine->function_object(temp, transitions, LUA_TTABLE);
    VERIFY(result);
    for (luabind::iterator I(transitions), E; I != E; ++I)
    {
        luabind::object table = *I;
        if (luabind::type(table) != LUA_TTABLE)
        {
            VERIFY(luabind::type(table) != LUA_TNIL);
            continue;
        }

        shared_str vertex_0_id = parse_vertex(table, "vertex0", true);
        shared_str vertex_1_id = parse_vertex(table, "vertex1", false);
        float weight = parse_float(table, "weight");

        if (!m_transitions.vertex(vertex_0_id))
            m_transitions.add_vertex(Loki::EmptyType(), vertex_0_id);

        if (!m_transitions.vertex(vertex_1_id))
            m_transitions.add_vertex(Loki::EmptyType(), vertex_1_id);

        m_transitions.add_edge(vertex_0_id, vertex_1_id, weight);
        TransitionGraph::CEdge* edge = m_transitions.edge(vertex_0_id, vertex_1_id);
        load_actions(table, edge->data());
    }
}

void description::load_actions(luabind::object const& table, description::ActionsList& result)
{
    luabind::object actions;
    parse_table(table, "actions", actions);
    for (luabind::iterator I(actions), E; I != E; ++I)
    {
        luabind::object tmp = *I;
        transitions::action* action = new transitions::action(tmp);
        result.push_back(action);
    }
}

template <typename _data_type, typename _edge_weight_type, typename _vertex_id_type, typename _edge_data_type>
IC void delete_data(const CGraphAbstract<_data_type, _edge_weight_type, _vertex_id_type, _edge_data_type>& graph_)
{
    typedef CGraphAbstract<_data_type, _edge_weight_type, _vertex_id_type, _edge_data_type> Graph;

    Graph& graph = const_cast<Graph&>(graph_);

    using Vertices = typename Graph::VERTICES;
    using Edges = typename Graph::EDGES;

    Vertices& verts = graph.vertices();

    for (auto vi = verts.begin(); vi != verts.end(); ++vi)
    {
        typename Graph::CVertex* vert = (*vi).second;
        delete_data(vert->data());

        Edges& edges = const_cast<Edges&>(vert->edges());
        for (auto ei = edges.begin(); ei != edges.end(); ++ei)
        {
            typename Graph::CEdge& edge = (*ei);
            delete_data(edge.data());
        }
    }
}

description::~description()
{
    delete_data(m_loopholes);
    delete_data(m_transitions);
}

loophole const* description::get_loophole(shared_str const& loophole_id) const
{
    class id_predicate
    {
        shared_str m_id;

    public:
        IC id_predicate(shared_str const& id) : m_id(id) {}
        IC bool operator()(smart_cover::loophole const* loophole) const
        {
            return (m_id._get() == loophole->id()._get());
        }
    };

    Loopholes::const_iterator found = std::find_if(m_loopholes.begin(), m_loopholes.end(), id_predicate(loophole_id));
    if (found != m_loopholes.end())
        return (*found);

    return (0);
}
