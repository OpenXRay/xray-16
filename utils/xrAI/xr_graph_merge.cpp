////////////////////////////////////////////////////////////////////////////
//	Module 		: xr_graph_merge.cpp
//	Created 	: 25.01.2003
//  Modified 	: 25.01.2003
//	Author		: Dmitriy Iassenev
//	Description : Merging level graphs for off-line AI NPC computations
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../../xrcore/xr_ini.h"
#include "../../xrEngine/xrLevel.h"
#include "xrAI.h"
#include "xrServer_Objects_ALife_All.h"
#include "factory_api.h"
#include "xrCrossTable.h"
#include "level_graph.h"
#include "object_broker.h"
#include "xr_graph_merge.h"
#include "spawn_constructor_space.h"
#include "guid_generator.h"
#include "game_graph_builder.h"
#include <direct.h>

extern LPCSTR GAME_CONFIG;
extern LPCSTR LEVEL_GRAPH_NAME;

using namespace SpawnConstructorSpace;
using namespace ALife;

class CLevelGameGraph;

typedef struct tagSConnectionVertex {
	LPSTR		caConnectName;
	GameGraph::_GRAPH_ID	tGraphID;
	GameGraph::_GRAPH_ID	tOldGraphID;
	u32			dwLevelID;
} SConnectionVertex;

extern  HWND logWindow;

CGameGraph::CHeader				tGraphHeader;

class CCompareVertexPredicate {
public:
	IC bool operator()(LPCSTR S1, LPCSTR S2) const
	{
		return(xr_strcmp(S1,S2) < 0);
	}
};

u32 dwfGetIDByLevelName(CInifile *Ini, LPCSTR caLevelName)
{
	LPCSTR				N,V;
	for (u32 k = 0; Ini->r_line("levels",k,&N,&V); k++) {
		R_ASSERT3		(Ini->section_exist(N),"Fill section properly!",N);
		R_ASSERT3		(Ini->line_exist(N,"caption"),"Fill section properly!",N);
		R_ASSERT3		(Ini->line_exist(N,"id"),"Fill section properly!",N);
		if (!xr_strcmp(Ini->r_string_wb(N,"caption"),caLevelName))
			return		(Ini->r_u32(N,"id"));
	}
	return				(u32(-1));
}

DEFINE_MAP		(u32,					::CLevelGameGraph*,		GRAPH_P_MAP,			GRAPH_P_PAIR_IT);
DEFINE_MAP_PRED	(LPSTR,					SConnectionVertex,		VERTEX_MAP,				VERTEX_PAIR_IT,	CCompareVertexPredicate);

typedef struct tagSDynamicGraphVertex {
	Fvector						tLocalPoint;
	Fvector						tGlobalPoint;
	u32							tNodeID;
	u8							tVertexTypes[GameGraph::LOCATION_TYPE_COUNT];
	u32							tLevelID;
	u32							tNeighbourCount;
	u32							tDeathPointCount;
	u32							dwPointOffset;
	CGameGraph::CEdge			*tpaEdges;
} SDynamicGraphVertex;

DEFINE_VECTOR(SDynamicGraphVertex,		GRAPH_VERTEX_VECTOR,	GRAPH_VERTEX_IT);
DEFINE_VECTOR(CGameGraph::CEdge,	GRAPH_EDGE_VECTOR,		GRAPH_EDGE_IT);

class CLevelGameGraph {
public:
	GRAPH_VERTEX_VECTOR			m_tpVertices;
	CGameGraph::SLevel			m_tLevel;
	VERTEX_MAP					m_tVertexMap;
	u32							m_dwOffset;
	LEVEL_POINT_STORAGE			m_tpLevelPoints;
	CGameGraph					*m_tpGraph;
	CMemoryWriter				m_cross_table;

								CLevelGameGraph	(
									LPCSTR graph_file_name,
									LPCSTR raw_cross_table_file_name,
									CGameGraph::SLevel *tLevel,
									LPCSTR S,
									u32 dwOffset,
									u32 dwLevelID,
									CInifile *Ini
								)
	{
		m_tLevel				= *tLevel;
		m_dwOffset				= dwOffset;
		m_tpLevelPoints.clear	();
		
		FILE_NAME				caFileName;
		
		// loading graph
		xr_strcpy				(caFileName,graph_file_name);
		m_tpGraph				= xr_new<CGameGraph>(caFileName);

		xr_strcpy				(caFileName,raw_cross_table_file_name);
		CGameLevelCrossTable	*l_tpCrossTable = xr_new<CGameLevelCrossTable>(caFileName);

		CLevelGraph				*l_tpAI_Map = xr_new<CLevelGraph>(S);

		VERIFY2					(l_tpCrossTable->header().level_guid() == l_tpAI_Map->header().guid(), "cross table doesn't correspond to the AI-map, rebuild graph!");
		VERIFY2					(l_tpCrossTable->header().game_guid() == m_tpGraph->header().guid(), "cross table doesn't correspond to the graph, rebuild graph!");
		VERIFY2					(m_tpGraph->header().level(GameGraph::_LEVEL_ID(0)).guid() == l_tpAI_Map->header().guid(), "cross table doesn't correspond to the AI-map, rebuild graph!");

		VERIFY					(l_tpAI_Map->header().vertex_count() == l_tpCrossTable->header().level_vertex_count());
		VERIFY					(m_tpGraph->header().vertex_count() == l_tpCrossTable->header().game_vertex_count());

		tLevel->m_guid			= l_tpAI_Map->header().guid();

		{
			for (GameGraph::_GRAPH_ID i=0, n = m_tpGraph->header().vertex_count(); i<n; ++i)
				if ((!l_tpAI_Map->valid_vertex_id(m_tpGraph->vertex(i)->level_vertex_id()) ||
					(l_tpCrossTable->vertex(m_tpGraph->vertex(i)->level_vertex_id()).game_vertex_id() != i) ||
					!l_tpAI_Map->inside(m_tpGraph->vertex(i)->level_vertex_id(),m_tpGraph->vertex(i)->level_point()))) {
						Msg				("! Graph doesn't correspond to the cross table");
						R_ASSERT2		(false,"Graph doesn't correspond to the cross table");
				}
		}

		m_tpVertices.resize		(m_tpGraph->header().vertex_count());
		GRAPH_VERTEX_IT			B = m_tpVertices.begin();
		GRAPH_VERTEX_IT			I = B;
		GRAPH_VERTEX_IT			E = m_tpVertices.end();
		for ( ; I != E; I++) {
			(*I).tLocalPoint		= m_tpGraph->vertex(int(I - B))->level_point();
			(*I).tGlobalPoint.add	(m_tpGraph->vertex(int(I - B))->game_point(),m_tLevel.offset());
			(*I).tLevelID			= dwLevelID;
			(*I).tNodeID			= m_tpGraph->vertex(int(I - B))->level_vertex_id();
			Memory.mem_copy			((*I).tVertexTypes,m_tpGraph->vertex(int(I - B))->vertex_type(),GameGraph::LOCATION_TYPE_COUNT*sizeof(GameGraph::_LOCATION_ID));
			(*I).tNeighbourCount	= m_tpGraph->vertex(int(I - B))->edge_count();
			CGameGraph::const_iterator	b,i,e;
			m_tpGraph->begin		(int(I - B),i,e);
			(*I).tpaEdges			= (CGameGraph::CEdge*)xr_malloc((*I).tNeighbourCount*sizeof(CGameGraph::CEdge));
			b						= i;
			for ( ; i != e; ++i) {
				GameGraph::CEdge	&edge = (*I).tpaEdges[i - b];
				edge				= *i;
				VERIFY				((edge.vertex_id() + dwOffset) < (u32(1) << (8*sizeof(GameGraph::_GRAPH_ID))));
				edge.m_vertex_id	= (GameGraph::_GRAPH_ID)(edge.m_vertex_id + dwOffset);
			}
			(*I).dwPointOffset		= 0;
			vfGenerateDeathPoints	(int(I - B),l_tpCrossTable,l_tpAI_Map,(*I).tDeathPointCount);
		}

		xr_delete					(l_tpCrossTable);
		xr_delete					(l_tpAI_Map);
		
		// updating cross-table
		{
			xr_strcpy				(caFileName,raw_cross_table_file_name);
			CGameLevelCrossTable	*tpCrossTable = xr_new<CGameLevelCrossTable>(caFileName);
			xr_vector<CGameLevelCrossTable::CCell> tCrossTableUpdate;
			tCrossTableUpdate.resize(tpCrossTable->header().level_vertex_count());
			for (int i=0; i<(int)tpCrossTable->header().level_vertex_count(); i++) {
				tCrossTableUpdate[i] = tpCrossTable->vertex(i);
				VERIFY				(u32(tCrossTableUpdate[i].tGraphIndex) < tpCrossTable->header().game_vertex_count());
				tCrossTableUpdate[i].tGraphIndex = tCrossTableUpdate[i].tGraphIndex + (GameGraph::_GRAPH_ID)dwOffset;
			}

			CGameLevelCrossTable::CHeader	tCrossTableHeader;

			tCrossTableHeader.dwVersion			= XRAI_CURRENT_VERSION;
			tCrossTableHeader.dwNodeCount		= tpCrossTable->m_tCrossTableHeader.dwNodeCount;
			tCrossTableHeader.dwGraphPointCount = tpCrossTable->m_tCrossTableHeader.dwGraphPointCount;
			tCrossTableHeader.m_level_guid		= tpCrossTable->m_tCrossTableHeader.m_level_guid;
			tCrossTableHeader.m_game_guid		= tGraphHeader.m_guid;

			xr_delete			(tpCrossTable);

			m_cross_table.w(&tCrossTableHeader,sizeof(tCrossTableHeader));
			for (int i=0; i<(int)tCrossTableHeader.dwNodeCount; i++)
				m_cross_table.w(&(tCrossTableUpdate[i]),sizeof(tCrossTableUpdate[i]));
		}

		// fill vertex map
		{
			string_path								fName;
			strconcat								(sizeof(fName),fName,S,"level.spawn");
			IReader									*F = FS.r_open(fName);
			u32										id;
			IReader									*O = F->open_chunk_iterator(id);
			for (int i=0; O; O = F->open_chunk_iterator(id,O))	{
				NET_Packet							P;
				P.B.count							= O->length();
				O->r								(P.B.data,P.B.count);
				u16									ID;
				P.r_begin							(ID);
				R_ASSERT							(M_SPAWN==ID);
				P.r_stringZ							(fName);
				CSE_Abstract						*E = F_entity_Create(fName);
				R_ASSERT3							(E,"Can't create entity.",fName);
//				E->Spawn_Read						(P);
				CSE_ALifeGraphPoint					*tpGraphPoint = smart_cast<CSE_ALifeGraphPoint*>(E);
				if (tpGraphPoint) {
					E->Spawn_Read					(P);

					Fvector							tVector;
					tVector							= tpGraphPoint->o_Position;
					GameGraph::_GRAPH_ID			tGraphID = GameGraph::_GRAPH_ID(-1);
					float							fMinDistance = 1000000.f;
					{
						GRAPH_VERTEX_IT					B = m_tpVertices.begin();
						GRAPH_VERTEX_IT					I = B;
						GRAPH_VERTEX_IT					E = m_tpVertices.end();
						for ( ; I != E; I++) {
							float fDistance = (*I).tLocalPoint.distance_to(tVector);
							if (fDistance < fMinDistance) {
								fMinDistance	= fDistance;
								tGraphID		= GameGraph::_GRAPH_ID(I - B);
								if (fMinDistance < EPS_L)
									break;
							}
						}
					}
					if (fMinDistance < EPS_L) {
						SConnectionVertex				T;
						LPSTR							S;
						S								= xr_strdup(tpGraphPoint->name_replace());
						T.caConnectName					= xr_strdup(*tpGraphPoint->m_caConnectionPointName);
						T.dwLevelID						= dwfGetIDByLevelName(Ini,*tpGraphPoint->m_caConnectionLevelName);
//						T.tGraphID						= (GameGraph::_GRAPH_ID)i;
//						T.tOldGraphID					= tGraphID;
						T.tOldGraphID					= (GameGraph::_GRAPH_ID)i;
						T.tGraphID						= tGraphID;

						bool							ok = true;
						VERTEX_MAP::const_iterator		II = m_tVertexMap.begin();
						VERTEX_MAP::const_iterator		EE = m_tVertexMap.end();
						for ( ; II != EE; ++II)
							if (T.tOldGraphID == (*II).second.tOldGraphID) {
								ok						= false;
								Msg						("Graph point %s is removed,because it has the same position as some another graph point",E->name_replace());
								break;
							}

						if (ok) {
							m_tVertexMap.insert			(mk_pair(S,T));
							i++;
						}
					}
				}
				F_entity_Destroy					(E);
			}
			if (i != m_tpGraph->header().vertex_count())
				Msg									("Graph for the level %s doesn't correspond to the graph points from Level Editor! (%d : %d)",*m_tLevel.name(),i,m_tpGraph->header().vertex_count());
			
			VERTEX_MAP::const_iterator				I = m_tVertexMap.begin();
			VERTEX_MAP::const_iterator				E = m_tVertexMap.end();
			for ( ; I != E; ++I) {
				R_ASSERT3	(!xr_strlen((*I).second.caConnectName) || ((*I).second.tGraphID < m_tpVertices.size()),"Rebuild graph for the level",*m_tLevel.name());
			}

//			VERIFY3									(i == m_tpGraph->header().vertex_count(), "Rebuild graph for the level ",m_tLevel.name());
			O->close								();
			FS.r_close								(F);
		}
	};

	virtual							~CLevelGameGraph()
	{
		{
			GRAPH_VERTEX_IT			I = m_tpVertices.begin();
			GRAPH_VERTEX_IT			E = m_tpVertices.end();
			for ( ; I != E; I++)
				xr_free((*I).tpaEdges);
		}
		delete_data					(m_tVertexMap);
		xr_delete					(m_tpGraph);
	};

	void						vfAddEdge(u32 dwVertexNumber, CGameGraph::CEdge &tGraphEdge)
	{
		R_ASSERT(m_tpGraph->header().vertex_count() > dwVertexNumber);
		m_tpVertices[dwVertexNumber].tpaEdges = (CGameGraph::CEdge *)xr_realloc(m_tpVertices[dwVertexNumber].tpaEdges,sizeof(CGameGraph::CEdge)*++m_tpVertices[dwVertexNumber].tNeighbourCount);
		m_tpVertices[dwVertexNumber].tpaEdges[m_tpVertices[dwVertexNumber].tNeighbourCount - 1] = tGraphEdge;
	}

	void						vfSaveVertices(CMemoryWriter &tMemoryStream, u32 &dwOffset, u32 &dwPointOffset, LEVEL_POINT_STORAGE *tpLevelPoints)
	{
		GRAPH_VERTEX_IT			I = m_tpVertices.begin();
		GRAPH_VERTEX_IT			E = m_tpVertices.end();
		GameGraph::CVertex		tVertex;
		for ( ; I != E; I++) {
			tVertex.tLocalPoint		= (*I).tLocalPoint;
			tVertex.tGlobalPoint	= (*I).tGlobalPoint;
			tVertex.tNodeID			= (*I).tNodeID;
			Memory.mem_copy			(tVertex.tVertexTypes,(*I).tVertexTypes,GameGraph::LOCATION_TYPE_COUNT*sizeof(GameGraph::_LOCATION_ID));
			tVertex.tLevelID		= (*I).tLevelID;
			tVertex.dwEdgeOffset	= dwOffset;
			tVertex.dwPointOffset	= dwPointOffset;
		
			VERIFY					((*I).tNeighbourCount < (u32(1) << (8*sizeof(u8))));
			tVertex.tNeighbourCount = (u8)(*I).tNeighbourCount;

			VERIFY					((*I).tDeathPointCount < (u32(1) << (8*sizeof(u8))));
			tVertex.tDeathPointCount= (u8)(*I).tDeathPointCount;

			tMemoryStream.w			(&tVertex,sizeof(tVertex));
			dwOffset				+= (*I).tNeighbourCount*sizeof(CGameGraph::CEdge);
			dwPointOffset			+= (*I).tDeathPointCount*sizeof(CGameGraph::CLevelPoint);
		}
	};
	
	void						vfSaveEdges(CMemoryWriter &tMemoryStream)
	{
		GRAPH_VERTEX_IT			I = m_tpVertices.begin();
		GRAPH_VERTEX_IT			E = m_tpVertices.end();
		for ( ; I != E; I++)
			for (int i=0; i<(int)(*I).tNeighbourCount; i++)
				tMemoryStream.w	((*I).tpaEdges + i,sizeof(CGameGraph::CEdge));
	};

	void						save_cross_table	(IWriter &stream)
	{
		stream.w_u32			(m_cross_table.size() + sizeof(u32));
		m_cross_table.seek		(0);
		stream.w				(m_cross_table.pointer(),m_cross_table.size());
		m_cross_table.clear		();
	}

	u32							dwfGetEdgeCount()
	{
		u32						l_dwResult = 0;
		GRAPH_VERTEX_IT			I = m_tpVertices.begin();
		GRAPH_VERTEX_IT			E = m_tpVertices.end();
		for ( ; I != E; I++)
			l_dwResult += (*I).tNeighbourCount;
		return					(l_dwResult);
	}

	u32							dwfGetDeathPointCount()
	{
		u32						l_dwResult = 0;
		GRAPH_VERTEX_IT			I = m_tpVertices.begin();
		GRAPH_VERTEX_IT			E = m_tpVertices.end();
		for ( ; I != E; I++)
			l_dwResult += (*I).tDeathPointCount;
		return					(l_dwResult);
	}

	void						vfGenerateDeathPoints(int iGraphIndex, CGameLevelCrossTable *tpCrossTable, CLevelGraph *tpAI_Map, u32 &dwDeathPointCount)
	{
		xr_vector<u32>			l_dwaNodes;
		l_dwaNodes.clear		();
		{
			for (u32 i=0, n = tpCrossTable->m_tCrossTableHeader.dwNodeCount; i<n; i++)
				if (tpCrossTable->m_tpaCrossTable[i].tGraphIndex == iGraphIndex)
					l_dwaNodes.push_back(i);
		}

		R_ASSERT2				(!l_dwaNodes.empty(),"Can't create at least one death point for specified graph point");

		std::random_shuffle		(l_dwaNodes.begin(),l_dwaNodes.end());

		u32						m = l_dwaNodes.size() > 10 ? _min(iFloor(.1f*l_dwaNodes.size()),255) : l_dwaNodes.size(), l_dwStartIndex = m_tpLevelPoints.size();
		m_tpLevelPoints.resize	(l_dwStartIndex + m);
		LEVEL_POINT_STORAGE::iterator I = m_tpLevelPoints.begin() + l_dwStartIndex;
		LEVEL_POINT_STORAGE::iterator E = m_tpLevelPoints.end();
		xr_vector<u32>::iterator		 i = l_dwaNodes.begin();

		dwDeathPointCount		= m;

		for ( ; I != E; I++, i++) {
			(*I).tNodeID	= *i;
			(*I).tPoint		= tpAI_Map->vertex_position(*i);
			(*I).fDistance	= tpCrossTable->vertex(*i).distance();
		}
	}

};

class CGraphMerger {
public:
	CGraphMerger(
		LPCSTR game_graph_id,
		LPCSTR name,
		bool rebuild
	);
};

void read_levels(CInifile *Ini, xr_set<CLevelInfo> &levels, bool rebuild_graph, xr_vector<LPCSTR> *needed_levels)
{
	LPCSTR				_N,V;
	string_path			caFileName, file_name;
	for (u32 k = 0; Ini->r_line("levels",k,&_N,&V); k++) {
		string256		N;
		xr_strcpy(N,_N);
		strlwr			(N);

		if (!Ini->section_exist(N)) {
			Msg			("! There is no section %s in the %s!",N,GAME_CONFIG);
			continue;
		}

		if (!Ini->line_exist(N,"name")) {
			Msg			("! There is no line \"name\" in the section %s!",N);
			continue;
		}
		
		if (!Ini->line_exist(N,"id")) {
			Msg			("! There is no line \"id\" in the section %s!",N);
			continue;
		}

		if (!Ini->line_exist(N,"offset")) {
			Msg			("! There is no line \"offset\" in the section %s!",N);
			continue;
		}

		u8				id = Ini->r_u8(N,"id");
		LPCSTR			_S = Ini->r_string(N,"name");
		string256		S;
		xr_strcpy		(S,_S);
		strlwr			(S);

		if (needed_levels) {
			bool		found = false;
			xr_vector<LPCSTR>::const_iterator	I = needed_levels->begin();
			xr_vector<LPCSTR>::const_iterator	E = needed_levels->end();
			for ( ; I != E; ++I)
				if (!xr_strcmp(*I,S)) {
					found	= true;
					break;
				}

			if (!found)
				continue;
		}

		{
			bool		ok = true;
			xr_set<CLevelInfo>::const_iterator	I = levels.begin();
			xr_set<CLevelInfo>::const_iterator	E = levels.end();
			for ( ; I != E; ++I) {
				if (!xr_strcmp((*I).m_section,N)) {
					Msg	("! Duplicated line %s in section \"levels\" in the %s",N,GAME_CONFIG);
					ok	= false;
					break;
				}
				if (!xr_strcmp((*I).m_name,S)) {
					Msg	("! Duplicated level name %s in the %s, sections %s, %s",S,GAME_CONFIG,*(*I).m_section,N);
					ok	= false;
					break;
				}
				if ((*I).m_id == id) {
					Msg	("! Duplicated level id %d in the %s, section %s, level %s",id,GAME_CONFIG,N,S);
					ok	= false;
					break;
				}
			}
			if (!ok)
				continue;
		}
		IReader			*reader;
		// ai
		strconcat		(sizeof(caFileName),caFileName,S,"\\",LEVEL_GRAPH_NAME);
		FS.update_path	(file_name,"$game_levels$",caFileName);
		if (!FS.exist(file_name)) {
			Msg			("! There is no ai-map for the level %s! (level is not included into the game graph)",S);
			continue;
		}
		
		{
			reader					= FS.r_open	(file_name);
			CLevelGraph::CHeader	header;
			reader->r				(&header,sizeof(header));
			FS.r_close				(reader);
			if (header.version() != XRAI_CURRENT_VERSION) {
				Msg					("! AI-map for the level %s is incompatible (version mismatch)! (level is not included into the game graph)",S);
				continue;
			}
		}

		levels.insert	(CLevelInfo(id,S,Ini->r_fvector3(N,"offset"),N));
	}
}

extern void xrBuildGraph(LPCSTR name);

LPCSTR generate_temp_file_name	(LPCSTR header0, LPCSTR header1, string_path& buffer)
{
	string_path			path;
	FS.update_path		(path,"$app_data_root$","temp");
	xr_strcat			(path,sizeof(path),"\\");

	_mkdir				(path);
	
	strconcat			(sizeof(buffer),buffer,path,header0,header1);
	return				(buffer);
}

void fill_needed_levels	(LPSTR levels, xr_vector<LPCSTR> &result)
{
	LPSTR					I = levels;
	for (LPSTR J = I; ; ++I) {
		if (*I != ',') {
			if (*I)
				continue;

			result.push_back(J);
			break;
		}

		*I					= 0;
		result.push_back	(J);
		J					= I + 1;
	}
}

CGraphMerger::CGraphMerger(
		LPCSTR game_graph_id,
		LPCSTR name,
		bool rebuild
	)
{
	// load all the graphs
	Phase("Processing level graphs");
	
	CInifile *Ini = xr_new<CInifile>(INI_FILE);
	if (!Ini->section_exist("levels"))
		THROW(false);
	R_ASSERT						(Ini->section_exist("levels"));

	tGraphHeader.m_guid				= generate_guid();

	GRAPH_P_MAP						tpGraphs;
	string4096						S1, S2;
	CGameGraph::SLevel				tLevel;
	u32								dwOffset = 0;
	u32								l_dwPointOffset = 0;
	LEVEL_POINT_STORAGE				l_tpLevelPoints;
	l_tpLevelPoints.clear			();

	xr_set<CLevelInfo>				levels;
	xr_vector<LPCSTR>				needed_levels;
	string4096						levels_string;
	xr_strcpy						(levels_string,name);
	strlwr							(levels_string);
	fill_needed_levels				(levels_string,needed_levels);

	read_levels						(
		Ini,
		levels,
		rebuild,
		&needed_levels
	);

	xr_set<CLevelInfo>::const_iterator	I = levels.begin();
	xr_set<CLevelInfo>::const_iterator	E = levels.end();
	for ( ; I != E; ++I) {
		tLevel.m_offset				= (*I).m_offset;
		tLevel.m_name				= (*I).m_name;
		xr_strcpy					(S1,sizeof(S1),*(*I).m_name);
		strconcat					(sizeof(S2),S2,name,S1);
		strconcat					(sizeof(S1),S1,S2,"\\");
		tLevel.m_id					= (*I).m_id;
		tLevel.m_section			= (*I).m_section;
		Msg							("%9s %2d %s","level",tLevel.id(),*tLevel.m_name);
		string_path					_0, _1;
		generate_temp_file_name		("local_graph_",*tLevel.m_name,_0);
		generate_temp_file_name		("raw_cross_table_",*tLevel.m_name,_1);
		string_path					level_folder;
		FS.update_path				(level_folder,"$game_levels$",*tLevel.m_name);
		xr_strcat						(level_folder,"\\");
		CGameGraphBuilder().build_graph	(_0,_1,level_folder);
		::CLevelGameGraph			*tpLevelGraph = xr_new<::CLevelGameGraph>(
			_0,
			_1,
			&tLevel,
			level_folder,
			dwOffset,
			tLevel.id(),
			Ini
		);
		dwOffset					+= tpLevelGraph->m_tpGraph->header().vertex_count();
		R_ASSERT2					(tpGraphs.find(tLevel.id()) == tpGraphs.end(),"Level ids _MUST_ be different!");
		tpGraphs.insert				(mk_pair(tLevel.id(),tpLevelGraph));
		tGraphHeader.m_levels.insert(std::make_pair(tLevel.id(),tLevel));
	}
	
	R_ASSERT(tpGraphs.size());
	
	Phase("Adding interconnection points");
	{
		GRAPH_P_PAIR_IT				I = tpGraphs.begin();
		GRAPH_P_PAIR_IT				E = tpGraphs.end();
		for ( ; I != E; I++) {
			VERTEX_PAIR_IT			i = (*I).second->m_tVertexMap.begin();
			VERTEX_PAIR_IT			e = (*I).second->m_tVertexMap.end();
			for ( ; i != e; i++)
				if ((*i).second.caConnectName[0]) {
					GRAPH_P_PAIR_IT				K;
					VERTEX_PAIR_IT				M;
					CGameGraph::CEdge			tGraphEdge;
					SConnectionVertex			&tConnectionVertex = (*i).second;
					K							= tpGraphs.find(tConnectionVertex.dwLevelID);
					if (K == tpGraphs.end()) {
						Msg						("Cannot find level with level_id %d. Connection point will not be generated!",tConnectionVertex.dwLevelID);
						continue;
					}
					R_ASSERT					(K != tpGraphs.end());
					M							= (*K).second->m_tVertexMap.find(tConnectionVertex.caConnectName);
					if (M == (*K).second->m_tVertexMap.end()) {
						Msg						("Level %s with id %d has an INVALID connection point %s,\nwhich references to graph point %s on the level %s with id %d\n",*(*I).second->m_tLevel.name(),(*I).second->m_tLevel.id(),(*i).first,tConnectionVertex.caConnectName,*(*K).second->m_tLevel.name(),(*K).second->m_tLevel.id());
						R_ASSERT				(M != (*K).second->m_tVertexMap.end());
					}

//					if (!stricmp("l06_rostok",*(*I).second->m_tLevel.name())) {
//						__asm int 3;
//					}
					Msg							("Level %s with id %d has VALID connection point %s,\nwhich references to graph point %s on the level %s with id %d\n",*(*I).second->m_tLevel.name(),(*I).second->m_tLevel.id(),(*i).first,tConnectionVertex.caConnectName,*(*K).second->m_tLevel.name(),(*K).second->m_tLevel.id());

					VERIFY						(((*M).second.tGraphID + (*K).second->m_dwOffset) < (u32(1) << (8*sizeof(GameGraph::_GRAPH_ID))));
					tGraphEdge.m_vertex_id		= (GameGraph::_GRAPH_ID)((*M).second.tGraphID + (*K).second->m_dwOffset);
					VERIFY3						(tConnectionVertex.tGraphID < (*I).second->m_tpVertices.size(),"Rebuild graph for the level",*(*I).second->m_tLevel.name());
					VERIFY3						((*M).second.tGraphID < (*K).second->m_tpVertices.size(),"Rebuild graph for the level",*(*K).second->m_tLevel.name());
					tGraphEdge.m_path_distance	= (*I).second->m_tpVertices[tConnectionVertex.tGraphID].tGlobalPoint.distance_to((*K).second->m_tpVertices[(*M).second.tGraphID].tGlobalPoint);
					(*I).second->vfAddEdge		((*i).second.tGraphID,tGraphEdge);
//					tGraphEdge.dwVertexNumber	= (*i).second.tGraphID + (*I).second->m_dwOffset;
//					(*K).second->vfAddEdge		((*M).second.tGraphID,tGraphEdge);
				}
		}
	}
	// counting edges
	{
		tGraphHeader.m_edge_count			= 0;
		tGraphHeader.m_death_point_count	= 0;
		GRAPH_P_PAIR_IT						I = tpGraphs.begin();
		GRAPH_P_PAIR_IT						E = tpGraphs.end();
		for ( ; I != E; I++) {
			VERIFY							((u32(tGraphHeader.m_edge_count) + (*I).second->dwfGetEdgeCount()) < (u32(1) << (8*sizeof(GameGraph::_GRAPH_ID))));
			tGraphHeader.m_edge_count		+= (GameGraph::_GRAPH_ID)(*I).second->dwfGetEdgeCount();
			tGraphHeader.m_death_point_count+= (*I).second->dwfGetDeathPointCount();
		}
	}

	///////////////////////////////////////////////////
	
	// save all the graphs
	Phase("Saving graph being merged");
	CMemoryWriter				F;
	tGraphHeader.m_version		= XRAI_CURRENT_VERSION;
	VERIFY						(dwOffset < (u32(1) << (8*sizeof(GameGraph::_GRAPH_ID))));
	tGraphHeader.m_vertex_count	= (GameGraph::_GRAPH_ID)dwOffset;
	tGraphHeader.save			(&F);

	u32							vertex_count = 0;
	dwOffset					*= sizeof(CGameGraph::CVertex);
	u32							l_dwOffset = F.size();
	l_dwPointOffset				= dwOffset + tGraphHeader.edge_count()*sizeof(CGameGraph::CEdge);
	u32							l_dwStartPointOffset = l_dwPointOffset;
	{
		GRAPH_P_PAIR_IT			I = tpGraphs.begin();
		GRAPH_P_PAIR_IT			E = tpGraphs.end();
		for ( ; I != E; I++) {
			(*I).second->vfSaveVertices	(F,dwOffset,l_dwPointOffset,&l_tpLevelPoints);
			vertex_count		+= (*I).second->m_tpGraph->header().vertex_count();
		}
	}
	{
		GRAPH_P_PAIR_IT			I = tpGraphs.begin();
		GRAPH_P_PAIR_IT			E = tpGraphs.end();
		for ( ; I != E; I++)
			(*I).second->vfSaveEdges(F);
	}
	{
		l_tpLevelPoints.clear	();
		GRAPH_P_PAIR_IT			I = tpGraphs.begin();
		GRAPH_P_PAIR_IT			E = tpGraphs.end();
		for ( ; I != E; I++)
			l_tpLevelPoints.insert(l_tpLevelPoints.end(),(*I).second->m_tpLevelPoints.begin(),(*I).second->m_tpLevelPoints.end());
	}
	R_ASSERT2						(l_dwStartPointOffset == F.size() - l_dwOffset,"Graph file format is corrupted");
	{
		LEVEL_POINT_STORAGE::const_iterator	I = l_tpLevelPoints.begin();
		LEVEL_POINT_STORAGE::const_iterator	E = l_tpLevelPoints.end();
		for ( ; I != E; ++I)
			save_data				(*I,F);
	}
	{
		GRAPH_P_PAIR_IT			I = tpGraphs.begin();
		GRAPH_P_PAIR_IT			E = tpGraphs.end();
		for ( ; I != E; I++) {
			Msg					("cross_table offset: %d",F.size());
			(*I).second->save_cross_table	(F);
		}
	}
	
	string256						l_caFileName;
	xr_strcpy							(l_caFileName,game_graph_id);
	F.save_to						(l_caFileName);

	// free all the graphs
	Phase("Freeing resources being allocated");
	{
		GRAPH_P_PAIR_IT				I = tpGraphs.begin();
		GRAPH_P_PAIR_IT				E = tpGraphs.end();
		for ( ; I != E; I++)
			xr_free((*I).second);
	}
	xr_delete						(Ini);
}

void xrMergeGraphs(
		LPCSTR game_graph_id,
		LPCSTR name,
		bool rebuild
	)
{
	CGraphMerger	A(
		game_graph_id,
		name,
		rebuild
	);
}
