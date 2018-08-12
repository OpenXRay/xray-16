////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_spawn_registry.cpp
//	Created 	: 15.01.2003
//  Modified 	: 12.05.2004
//	Author		: Dmitriy Iassenev
//	Description : ALife spawn registry
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "alife_spawn_registry.h"
#include "Common/object_broker.h"
#include "game_base.h"
#include "ai_space.h"
#include "xrAICore/Navigation/game_graph.h"

#pragma warning(push)
#pragma warning(disable : 4995)
#include <malloc.h>
#pragma warning(pop)

CALifeSpawnRegistry::CALifeSpawnRegistry(LPCSTR section)
{
    m_spawn_name = "";
    seed(u32(CPU::QPC() & 0xffffffff));
    m_game_graph = nullptr;
    m_chunk = nullptr;
    m_file = nullptr;
}

CALifeSpawnRegistry::~CALifeSpawnRegistry()
{
    xr_delete(m_game_graph);
    m_chunk->close();
    FS.r_close(m_file);
}

void CALifeSpawnRegistry::save(IWriter& memory_stream)
{
    Msg("* Saving spawns...");
    memory_stream.open_chunk(SPAWN_CHUNK_DATA);

    memory_stream.open_chunk(0);
    memory_stream.w_stringZ(m_spawn_name);
    memory_stream.w(&header().guid(), sizeof(header().guid()));
    memory_stream.close_chunk();

    memory_stream.open_chunk(1);
    save_updates(memory_stream);
    memory_stream.close_chunk();

    memory_stream.close_chunk();
}

void CALifeSpawnRegistry::load(IReader& file_stream, LPCSTR game_name)
{
    R_ASSERT(FS.exist(game_name));

    IReader *chunk, *chunk0;
    Msg("* Loading spawn registry...");
    R_ASSERT2(file_stream.find_chunk(SPAWN_CHUNK_DATA), "Cannot find chunk SPAWN_CHUNK_DATA!");
    chunk0 = file_stream.open_chunk(SPAWN_CHUNK_DATA);

    xrGUID guid;
    chunk = chunk0->open_chunk(0);
    VERIFY(chunk);
    chunk->r_stringZ(m_spawn_name);
    chunk->r(&guid, sizeof(guid));
    chunk->close();

    string_path file_name;
    bool file_exists = !!FS.exist(file_name, "$game_spawn$", *m_spawn_name, ".spawn");
    R_ASSERT3(file_exists, "Can't find spawn file:", *m_spawn_name);

    VERIFY(!m_file);
    m_file = FS.r_open(file_name);
    load(*m_file, &guid);

    chunk0->close();
}

void CALifeSpawnRegistry::load(LPCSTR spawn_name)
{
    Msg("* Loading spawn registry...");
    m_spawn_name = spawn_name;
    string_path file_name;
    R_ASSERT3(FS.exist(file_name, "$game_spawn$", *m_spawn_name, ".spawn"), "Can't find spawn file:", *m_spawn_name);

    VERIFY(!m_file);
    m_file = FS.r_open(file_name);
    load(*m_file);
}

using lua_State = struct lua_State;
struct dummy
{
    int count;
    lua_State* state;
    int ref;
};

static bool ignore_save_incompatibility() { return (!!strstr(Core.Params, "-ignore_save_incompatibility")); }
void CALifeSpawnRegistry::load(IReader& file_stream, xrGUID* save_guid)
{
    IReader* chunk;
    chunk = file_stream.open_chunk(0);
    m_header.load(*chunk);
    chunk->close();
    R_ASSERT2(!save_guid || (*save_guid == header().guid()) || ignore_save_incompatibility(),
        "Saved game doesn't correspond to the spawn : DELETE SAVED GAME!");

    chunk = file_stream.open_chunk(1);
    m_spawns.load(*chunk);
    chunk->close();

#if 0
	SPAWN_GRAPH::vertex_iterator			I = m_spawns.vertices().begin();
	SPAWN_GRAPH::vertex_iterator			E = m_spawns.vertices().end();
	for ( ; I != E; ++I) {
		luabind::wrap_base		*base = smart_cast<luabind::wrap_base*>(&(*I).second->data()->object());
		if (!base)
			continue;

		if (xr_strcmp((*I).second->data()->object().name_replace(),"rostok_stalker_outfit"))
			continue;

		dummy					*_dummy = (dummy*)((void*)base->m_self.m_impl);
		lua_State				**_state = &_dummy->state;
		Msg						("0x%08x",*(int*)&_state);
		break;
	}
#endif

    chunk = file_stream.open_chunk(2);
    load_data(m_artefact_spawn_positions, *chunk);
    chunk->close();

    chunk = file_stream.open_chunk(3);
    R_ASSERT2(chunk, "Spawn version mismatch - REBUILD SPAWN!");
    ai().patrol_path_storage(*chunk);
    chunk->close();

    VERIFY(!m_chunk);
    m_chunk = file_stream.open_chunk(4);
    R_ASSERT2(m_chunk, "Spawn version mismatch - REBUILD SPAWN!");

    VERIFY(!m_game_graph);
    m_game_graph = new CGameGraph(*m_chunk);
    ai().SetGameGraph(m_game_graph);

    R_ASSERT2((header().graph_guid() == ai().game_graph().header().guid()) || ignore_save_incompatibility(),
        "Spawn doesn't correspond to the graph : REBUILD SPAWN!");

    build_story_spawns();

    build_root_spawns();

    Msg("* %d spawn points are successfully loaded", m_spawns.vertex_count());
}

void CALifeSpawnRegistry::save_updates(IWriter& stream)
{
    SPAWN_GRAPH::vertex_iterator I = m_spawns.vertices().begin();
    SPAWN_GRAPH::vertex_iterator E = m_spawns.vertices().end();
    for (; I != E; ++I)
    {
        stream.open_chunk((*I).second->vertex_id());
        (*I).second->data()->save_update(stream);
        stream.close_chunk();
    }
}

void CALifeSpawnRegistry::load_updates(IReader& stream)
{
    u32 vertex_id;
    for (IReader* chunk = stream.open_chunk_iterator(vertex_id); chunk;
         chunk = stream.open_chunk_iterator(vertex_id, chunk))
    {
        VERIFY(u32(ALife::_SPAWN_ID(-1)) > vertex_id);
        const SPAWN_GRAPH::CVertex* vertex = m_spawns.vertex(ALife::_SPAWN_ID(vertex_id));
        VERIFY(vertex);
        vertex->data()->load_update(*chunk);
    }
}

void CALifeSpawnRegistry::build_root_spawns()
{
    m_temp0.clear();
    m_temp1.clear();

    {
        SPAWN_GRAPH::const_vertex_iterator I = m_spawns.vertices().begin();
        SPAWN_GRAPH::const_vertex_iterator E = m_spawns.vertices().end();
        for (; I != E; ++I)
            m_temp0.push_back((*I).second->vertex_id());
    }

    {
        SPAWN_GRAPH::const_vertex_iterator I = m_spawns.vertices().begin();
        SPAWN_GRAPH::const_vertex_iterator E = m_spawns.vertices().end();
        for (; I != E; ++I)
        {
            SPAWN_GRAPH::const_iterator i = (*I).second->edges().begin();
            SPAWN_GRAPH::const_iterator e = (*I).second->edges().end();
            for (; i != e; ++i)
                m_temp1.push_back((*i).vertex_id());
        }
    }

    process_spawns(m_temp0);
    process_spawns(m_temp1);

    m_spawn_roots.resize(m_temp0.size() + m_temp1.size());
    xr_vector<ALife::_SPAWN_ID>::iterator I =
        std::set_difference(m_temp0.begin(), m_temp0.end(), m_temp1.begin(), m_temp1.end(), m_spawn_roots.begin());

    m_spawn_roots.erase(I, m_spawn_roots.end());
}

void CALifeSpawnRegistry::build_story_spawns()
{
    SPAWN_GRAPH::const_vertex_iterator I = m_spawns.vertices().begin();
    SPAWN_GRAPH::const_vertex_iterator E = m_spawns.vertices().end();
    for (; I != E; ++I)
    {
        CSE_ALifeObject* object = smart_cast<CSE_ALifeObject*>(&(*I).second->data()->object());
        VERIFY(object);
        if (object->m_spawn_story_id == INVALID_SPAWN_STORY_ID)
            continue;

        m_spawn_story_ids.insert(std::make_pair(object->m_spawn_story_id, (*I).first));
    }
}
