////////////////////////////////////////////////////////////////////////////
//	Module 		: xrCrossTable.h
//	Created 	: 25.01.2003
//  Modified 	: 25.01.2003
//	Author		: Dmitriy Iassenev
//	Description : Building cross table for AI nodes
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/game_graph.h"
#include "xrAICore/Navigation/level_graph.h"

#define CROSS_TABLE_NAME_RAW "level.gct.raw"

using FILE_NAME = char[_MAX_PATH];

extern const pcstr GAME_LEVEL_GRAPH;

extern void xrBuildCrossTable(LPCSTR caProjectName);
extern void vfRecurseMark(const CLevelGraph& tMap, xr_vector<bool>& tMarks, u32 dwStartNodeID);
