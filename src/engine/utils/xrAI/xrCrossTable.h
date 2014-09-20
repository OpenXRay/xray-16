////////////////////////////////////////////////////////////////////////////
//	Module 		: xrCrossTable.h
//	Created 	: 25.01.2003
//  Modified 	: 25.01.2003
//	Author		: Dmitriy Iassenev
//	Description : Building cross table for AI nodes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "../../xrEngine/xrLevel.h"
#include "alife_space.h"
#include "game_level_cross_table.h"
#include "game_graph.h"
#include "level_graph.h"

#define CROSS_TABLE_NAME_RAW	"level.gct.raw"

typedef	char FILE_NAME[_MAX_PATH];

extern LPCSTR GAME_LEVEL_GRAPH;

extern void xrBuildCrossTable	(LPCSTR	caProjectName);
extern void vfRecurseMark		(const CLevelGraph &tMap, xr_vector<bool> &tMarks, u32 dwStartNodeID);
