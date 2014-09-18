////////////////////////////////////////////////////////////////////////////
//	Module 		: spawn_patcher.h
//	Created 	: 12.06.2007
//  Modified 	: 12.06.2007
//	Author		: Dmitriy Iassenev
//	Description : spawn patcher class
////////////////////////////////////////////////////////////////////////////

#ifndef SPAWN_PATCHER_H
#define SPAWN_PATCHER_H

struct xrGUID;

class spawn_patcher {
private:
	static	bool		spawn_guid		(LPCSTR spawn_id, xrGUID &result);
	static	bool		process_header	(IReader &reader, IWriter &writer, xrGUID &previous_spawn_guid);
	static	bool		process_spawns	(IReader &reader, IWriter &writer);
	static	bool		process_level	(IReader &reader, IWriter &writer);
	static	bool		process_patrol	(IReader &reader, IWriter &writer);

public:
						spawn_patcher	(LPCSTR new_spawn_id, LPCSTR previous_spawn_id);
};

#endif // SPAWN_PATCHER_H