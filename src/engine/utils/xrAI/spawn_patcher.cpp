////////////////////////////////////////////////////////////////////////////
//	Module 		: spawn_patcher.cpp
//	Created 	: 12.06.2007
//  Modified 	: 12.06.2007
//	Author		: Dmitriy Iassenev
//	Description : spawn patcher class
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#if 0
#	include "spawn_patcher.h"
#	include "game_graph.h"
#	include "game_spawn_constructor.h"
#	include "patrol_path_storage.h"

struct reader_guard {
	IReader *m_reader;

	IC		reader_guard			(IReader *reader) :
		m_reader			(reader)
	{
	}
	
	IC		~reader_guard			()
	{
		FS.r_close			(m_reader);
	}
};

spawn_patcher::spawn_patcher		(LPCSTR new_spawn_id, LPCSTR previous_spawn_id)
{
	xrGUID					previous_spawn_guid;
	if (!spawn_guid(previous_spawn_id,previous_spawn_guid))
		return;

	string_path				file_name;
	if (!FS.exist(file_name, "$game_spawn$", new_spawn_id, ".spawn")) {
		Msg					("cannot open spawn file \"%s\"",file_name);
		return;
	}

	CMemoryWriter			writer;

	IReader					*reader = FS.r_open(file_name);
	reader_guard			__guard(reader);

	if (!process_header(*reader,writer,previous_spawn_guid))
		return;

	if (!process_spawns(*reader,writer))
		return;

	if (!process_level(*reader,writer))
		return;

	if (!process_patrol(*reader,writer))
		return;

	writer.save_to			(file_name);
}

bool spawn_patcher::spawn_guid		(LPCSTR spawn_id, xrGUID &result)
{
	string_path				file_name;
	if (!FS.exist(file_name, "$game_spawn$", spawn_id, ".spawn")) {
		Msg					("cannot open spawn file \"%s\"",file_name);
		return				(false);
	}

	IReader					*reader = FS.r_open(file_name);
	if (!reader) {
		Msg					("Cannot open file \"%s\"",file_name);
		return				(false);
	}
	reader_guard			__guard(reader);

	IReader					*chunk = reader->open_chunk(0);
	if (!chunk) {
		Msg					("Spawn [%s] is broken: cannot find header chunk",file_name);
		return				(false);
	}
	
	chunk->r_u32			();
	chunk->r				(&result,sizeof(result));
	chunk->close			();

	return					(true);
}

bool spawn_patcher::process_header	(IReader &reader, IWriter &writer, xrGUID &previous_spawn_guid)
{
	IReader					*chunk = reader.open_chunk(0);
	if (!chunk) {
		Msg					("spawn is broken: cannot find header chunk");
		return				(false);
	}

	typedef CGameSpawnConstructor::CSpawnHeader	CSpawnHeader;
	CSpawnHeader			header;
	header.m_version		= chunk->r_u32();
	chunk->r				(&header.m_guid,sizeof(header.m_guid));
	chunk->r				(&header.m_graph_guid,sizeof(header.m_graph_guid));
	header.m_spawn_count	= chunk->r_u32();
	header.m_level_count	= chunk->r_u32();
	
	chunk->close			();

	header.m_guid			= previous_spawn_guid;

	string256				game_graph_name;
	FS.update_path			(game_graph_name,"$game_data$",GRAPH_NAME);
	header.m_graph_guid		= CGameGraph(game_graph_name).header().guid();

	writer.open_chunk		(0);
	writer.w_u32			(header.m_version);
	writer.w				(&header.m_guid,sizeof(header.m_guid));
	writer.w				(&header.m_graph_guid,sizeof(header.m_graph_guid));
	writer.w_u32			(header.m_spawn_count);
	writer.w_u32			(header.m_level_count);
	writer.close_chunk		();

	return					(true);
}

bool spawn_patcher::process_spawns	(IReader &reader, IWriter &writer)
{
	IReader					*chunk = reader.open_chunk(1);
	if (!chunk) {
		Msg					("spawn is broken: cannot find header chunk");
		return				(false);
	}

	typedef CGameSpawnConstructor::SPAWN_GRAPH	SPAWN_GRAPH;
	SPAWN_GRAPH				spawns;
	
	load_data				(spawns,*chunk);
	chunk->close			();

	writer.open_chunk		(1);
	save_data				(spawns,writer);
	writer.close_chunk		();

	return					(true);
}

bool spawn_patcher::process_level	(IReader &reader, IWriter &writer)
{
	IReader					*chunk = reader.open_chunk(2);
	if (!chunk) {
		Msg					("spawn is broken: cannot find header chunk");
		return				(false);
	}

	typedef CGameSpawnConstructor::LEVEL_POINT_STORAGE	LEVEL_POINT_STORAGE;
	LEVEL_POINT_STORAGE		level_points;
	
	load_data				(level_points,*chunk);
	chunk->close			();

	writer.open_chunk		(2);
	save_data				(level_points,writer);
	writer.close_chunk		();

	return					(true);
}

bool spawn_patcher::process_patrol	(IReader &reader, IWriter &writer)
{
	IReader					*chunk = reader.open_chunk(3);
	if (!chunk) {
		Msg					("spawn is broken: cannot find header chunk");
		return				(false);
	}

	CPatrolPathStorage		patrol_storage;
	
	load_data				(patrol_storage,*chunk);
	chunk->close			();

	writer.open_chunk		(3);
	save_data				(patrol_storage,writer);
	writer.close_chunk		();

	return					(true);
}
#endif