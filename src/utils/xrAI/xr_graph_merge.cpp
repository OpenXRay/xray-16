////////////////////////////////////////////////////////////////////////////
//	Module 		: xr_graph_merge.cpp
//	Created 	: 25.01.2003
//  Modified 	: 25.01.2003
//	Author		: Dmitriy Iassenev
//	Description : Merging level graphs for off-line AI NPC computations
////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "xr_graph_merge.h"
#include "xrAI.h"
#include "factory_api.h"
#include "Common/object_broker.h"
#include "spawn_constructor_space.h"
#include "guid_generator.h"
#include "game_graph_builder.h"
#include "xrServerEntities/xrMessages.h"
#include <direct.h>
#include <random>

extern LPCSTR GAME_CONFIG;

using namespace SpawnConstructorSpace;
using namespace ALife;

typedef struct tagSConnectionVertex
{
    pstr caConnectName;
    GameGraph::_GRAPH_ID tGraphID;
    GameGraph::_GRAPH_ID tOldGraphID;
    u32 dwLevelID;
} SConnectionVertex;

extern HWND logWindow;

CGameGraph::CHeader tGraphHeader;

class CCompareVertexPredicate
{
public:
    IC bool operator()(LPCSTR S1, LPCSTR S2) const { return (xr_strcmp(S1, S2) < 0); }
};

u32 dwfGetIDByLevelName(CInifile* Ini, LPCSTR caLevelName)
{
    LPCSTR N, V;
    for (u32 k = 0; Ini->r_line("levels", k, &N, &V); k++)
    {
        R_ASSERT3(Ini->section_exist(N), "Fill section properly!", N);
        R_ASSERT3(Ini->line_exist(N, "caption"), "Fill section properly!", N);
        R_ASSERT3(Ini->line_exist(N, "id"), "Fill section properly!", N);
        if (!xr_strcmp(Ini->r_string_wb(N, "caption"), caLevelName))
            return (Ini->r_u32(N, "id"));
    }
    return (u32(-1));
}

using GRAPH_P_MAP = xr_map<u32, ::CLevelGameGraph*>;
using VERTEX_MAP = xr_map<pstr, SConnectionVertex, CCompareVertexPredicate>;

typedef struct tagSDynamicGraphVertex
{
    Fvector tLocalPoint;
    Fvector tGlobalPoint;
    u32 tNodeID;
    u8 tVertexTypes[GameGraph::LOCATION_TYPE_COUNT];
    u32 tLevelID;
    u32 tNeighbourCount;
    u32 tDeathPointCount;
    u32 dwPointOffset;
    CGameGraph::CEdge* tpaEdges;
} SDynamicGraphVertex;

using GRAPH_VERTEX_VECTOR = xr_vector<SDynamicGraphVertex>;
using GRAPH_EDGE_VECTOR = xr_vector<CGameGraph::CEdge>;

class CLevelGameGraph
{
public:
    GRAPH_VERTEX_VECTOR m_tpVertices;
    CGameGraph::SLevel m_tLevel;
    VERTEX_MAP m_tVertexMap;
    u32 m_dwOffset;
    LEVEL_POINT_STORAGE m_tpLevelPoints;
    CGameGraph* m_tpGraph;
    CMemoryWriter m_cross_table;

    CLevelGameGraph(LPCSTR graph_file_name, LPCSTR raw_cross_table_file_name, CGameGraph::SLevel* tLevel, LPCSTR S, u32 dwOffset, u32 dwLevelID, CInifile* Ini)
    {
        m_tLevel = *tLevel;
        m_dwOffset = dwOffset;
        m_tpLevelPoints.clear();

        string_path caFileName;

        // loading graph
        xr_strcpy(caFileName, graph_file_name);
        m_tpGraph = xr_new<CGameGraph>(caFileName);

        xr_strcpy(caFileName, raw_cross_table_file_name);
        auto l_tpCrossTable = xr_new<CGameLevelCrossTable>(caFileName);

        auto l_tpAI_Map = xr_new<CLevelGraph>(S);

        VERIFY2(l_tpCrossTable->header().level_guid() == l_tpAI_Map->header().guid(),
            "cross table doesn't correspond to the AI-map, rebuild graph!");
        VERIFY2(l_tpCrossTable->header().game_guid() == m_tpGraph->header().guid(),
            "cross table doesn't correspond to the graph, rebuild graph!");
        VERIFY2(m_tpGraph->header().level(GameGraph::_LEVEL_ID(0)).guid() == l_tpAI_Map->header().guid(),
            "cross table doesn't correspond to the AI-map, rebuild graph!");

        VERIFY(l_tpAI_Map->header().vertex_count() == l_tpCrossTable->header().level_vertex_count());
        VERIFY(m_tpGraph->header().vertex_count() == l_tpCrossTable->header().game_vertex_count());

        tLevel->m_guid = l_tpAI_Map->header().guid();

        {
            for (GameGraph::_GRAPH_ID i = 0, n = m_tpGraph->header().vertex_count(); i < n; ++i)
                if ((!l_tpAI_Map->valid_vertex_id(m_tpGraph->vertex(i)->level_vertex_id()) ||
                        (l_tpCrossTable->vertex(m_tpGraph->vertex(i)->level_vertex_id()).game_vertex_id() != i) ||
                        !l_tpAI_Map->inside(
                            m_tpGraph->vertex(i)->level_vertex_id(), m_tpGraph->vertex(i)->level_point())))
                {
                    Msg("! Graph doesn't correspond to the cross table");
                    R_ASSERT2(false, "Graph doesn't correspond to the cross table");
                }
        }

        m_tpVertices.resize(m_tpGraph->header().vertex_count());

        for (auto &i : m_tpVertices)
        {
            auto I = std::distance(&m_tpVertices.front(), &i);
            i.tLocalPoint = m_tpGraph->vertex(I)->level_point();
            i.tGlobalPoint.add(m_tpGraph->vertex(I)->game_point(), m_tLevel.offset());
            i.tLevelID = dwLevelID;
            i.tNodeID = m_tpGraph->vertex(I)->level_vertex_id();
            memcpy(i.tVertexTypes, m_tpGraph->vertex(I)->vertex_type(), GameGraph::LOCATION_TYPE_COUNT * sizeof(GameGraph::_LOCATION_ID));
            i.tNeighbourCount = m_tpGraph->vertex(I)->edge_count();
            CGameGraph::const_iterator b, j, e;
            m_tpGraph->begin(I, j, e);
            i.tpaEdges = (CGameGraph::CEdge*)xr_malloc(i.tNeighbourCount * sizeof(CGameGraph::CEdge));
            b = j;
            for (; j != e; ++j)
            {
                auto& edge = i.tpaEdges[j - b];
                edge = *j;
                VERIFY((edge.vertex_id() + dwOffset) < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
                edge.m_vertex_id = (GameGraph::_GRAPH_ID)(edge.m_vertex_id + dwOffset);
            }
            i.dwPointOffset = 0;
            vfGenerateDeathPoints(I, l_tpCrossTable, l_tpAI_Map, i.tDeathPointCount);
        }

        xr_delete(l_tpCrossTable);
        xr_delete(l_tpAI_Map);

        // updating cross-table
        {
            xr_strcpy(caFileName, raw_cross_table_file_name);
            auto tpCrossTable = xr_new<CGameLevelCrossTable>(caFileName);
            xr_vector<CGameLevelCrossTable::CCell> tCrossTableUpdate;
            tCrossTableUpdate.resize(tpCrossTable->header().level_vertex_count());
            for (int i = 0; i < (int)tpCrossTable->header().level_vertex_count(); i++)
            {
                tCrossTableUpdate[i] = tpCrossTable->vertex(i);
                VERIFY(u32(tCrossTableUpdate[i].tGraphIndex) < tpCrossTable->header().game_vertex_count());
                tCrossTableUpdate[i].tGraphIndex = tCrossTableUpdate[i].tGraphIndex + (GameGraph::_GRAPH_ID)dwOffset;
            }

            CGameLevelCrossTable::CHeader tCrossTableHeader;

            tCrossTableHeader.dwVersion = XRAI_CURRENT_VERSION;
            tCrossTableHeader.dwNodeCount = tpCrossTable->m_tCrossTableHeader.dwNodeCount;
            tCrossTableHeader.dwGraphPointCount = tpCrossTable->m_tCrossTableHeader.dwGraphPointCount;
            tCrossTableHeader.m_level_guid = tpCrossTable->m_tCrossTableHeader.m_level_guid;
            tCrossTableHeader.m_game_guid = tGraphHeader.m_guid;

            xr_delete(tpCrossTable);

            m_cross_table.w(&tCrossTableHeader, sizeof(tCrossTableHeader));
            for (int i = 0; i < (int)tCrossTableHeader.dwNodeCount; i++)
                m_cross_table.w(&(tCrossTableUpdate[i]), sizeof(tCrossTableUpdate[i]));
        }

        // fill vertex map
        {
            string_path fName;
            strconcat(sizeof(fName), fName, S, "level.spawn");
            auto F = FS.r_open(fName);
            u32 id;
            auto O = F->open_chunk_iterator(id);
            int vertexId = 0;
            for (; O; O = F->open_chunk_iterator(id, O))
            {
                NET_Packet P;
                P.B.count = O->length();
                O->r(P.B.data, P.B.count);
                u16 ID;
                P.r_begin(ID);
                R_ASSERT(M_SPAWN == ID);
                P.r_stringZ(fName);
                auto E = F_entity_Create(fName);
                R_ASSERT3(E, "Can't create entity.", fName);
                //E->Spawn_Read(P);
                auto tpGraphPoint = smart_cast<CSE_ALifeGraphPoint*>(E);
                if (tpGraphPoint)
                {
                    E->Spawn_Read(P);

                    auto tVector = tpGraphPoint->o_Position;
                    auto tGraphID = GameGraph::_GRAPH_ID(-1);
                    float fMinDistance = 1000000.f;
                    {
                        for (auto &i : m_tpVertices)
                        {
                            float fDistance = i.tLocalPoint.distance_to(tVector);
                            if (fDistance < fMinDistance)
                            {
                                fMinDistance = fDistance;
                                tGraphID = GameGraph::_GRAPH_ID(std::distance(&m_tpVertices.front(), &i));
                                if (fMinDistance < EPS_L)
                                    break;
                            }
                        }
                    }
                    if (fMinDistance < EPS_L)
                    {
                        SConnectionVertex T;
                        pstr S;
                        S = xr_strdup(tpGraphPoint->name_replace());
                        T.caConnectName = xr_strdup(*tpGraphPoint->m_caConnectionPointName);
                        T.dwLevelID = dwfGetIDByLevelName(Ini, *tpGraphPoint->m_caConnectionLevelName);
                        //						T.tGraphID						= (GameGraph::_GRAPH_ID)vertexId;
                        //						T.tOldGraphID					= tGraphID;
                        T.tOldGraphID = (GameGraph::_GRAPH_ID)vertexId;
                        T.tGraphID = tGraphID;

                        bool ok = true;
                        for (auto &i : m_tVertexMap)
                        {
                            if (T.tOldGraphID == i.second.tOldGraphID)
                            {
                                ok = false;
                                Msg("Graph point %s is removed,because it has the same position as some another graph point", E->name_replace());
                                break;
                            }
                        }

                        if (ok)
                        {
                            m_tVertexMap.insert(std::make_pair(S, T));
                            vertexId++;
                        }
                    }
                }
                F_entity_Destroy(E);
            }
            if (vertexId != m_tpGraph->header().vertex_count())
                Msg("Graph for the level %s doesn't correspond to the graph points from Level Editor! (%d : %d)",
                    *m_tLevel.name(), vertexId, m_tpGraph->header().vertex_count());

            for (auto &i : m_tVertexMap)
                R_ASSERT3(!xr_strlen(i.second.caConnectName) || (i.second.tGraphID < m_tpVertices.size()), "Rebuild graph for the level", *m_tLevel.name());

            //VERIFY3(vertexId==m_tpGraph->header().vertex_count(), "Rebuild graph for the level",m_tLevel.name());
            O->close();
            FS.r_close(F);
        }
    };

    virtual ~CLevelGameGraph()
    {
        for (auto &i : m_tpVertices)
            xr_free(i.tpaEdges);

        delete_data(m_tVertexMap);
        xr_delete(m_tpGraph);
    };

    void vfAddEdge(u32 dwVertexNumber, CGameGraph::CEdge& tGraphEdge)
    {
        R_ASSERT(m_tpGraph->header().vertex_count() > dwVertexNumber);
        m_tpVertices[dwVertexNumber].tpaEdges = (CGameGraph::CEdge*)xr_realloc(m_tpVertices[dwVertexNumber].tpaEdges,
            sizeof(CGameGraph::CEdge) * ++m_tpVertices[dwVertexNumber].tNeighbourCount);
        m_tpVertices[dwVertexNumber].tpaEdges[m_tpVertices[dwVertexNumber].tNeighbourCount - 1] = tGraphEdge;
    }

    void vfSaveVertices(
        CMemoryWriter& tMemoryStream, u32& dwOffset, u32& dwPointOffset, LEVEL_POINT_STORAGE* tpLevelPoints)
    {
        GameGraph::CGameVertex tVertex;

        for (auto &i : m_tpVertices)
        {
            tVertex.tLocalPoint = i.tLocalPoint;
            tVertex.tGlobalPoint = i.tGlobalPoint;
            tVertex.tNodeID = i.tNodeID;
            memcpy(tVertex.tVertexTypes, i.tVertexTypes,
                GameGraph::LOCATION_TYPE_COUNT * sizeof(GameGraph::_LOCATION_ID));
            tVertex.tLevelID = i.tLevelID;
            tVertex.dwEdgeOffset = dwOffset;
            tVertex.dwPointOffset = dwPointOffset;

            VERIFY(i.tNeighbourCount < (u32(1) << (8 * sizeof(u8))));
            tVertex.tNeighbourCount = (u8)i.tNeighbourCount;

            VERIFY(i.tDeathPointCount < (u32(1) << (8 * sizeof(u8))));
            tVertex.tDeathPointCount = (u8)i.tDeathPointCount;

            tMemoryStream.w(&tVertex, sizeof(tVertex));
            dwOffset += i.tNeighbourCount * sizeof(CGameGraph::CEdge);
            dwPointOffset += i.tDeathPointCount * sizeof(CGameGraph::CLevelPoint);
        }
    };

    void vfSaveEdges(CMemoryWriter& tMemoryStream)
    {
        for (auto &i : m_tpVertices)
            for (int j = 0; j < (int)i.tNeighbourCount; j++)
                tMemoryStream.w(i.tpaEdges + j, sizeof(CGameGraph::CEdge));
    };

    void save_cross_table(IWriter& stream)
    {
        stream.w_u32(m_cross_table.size() + sizeof(u32));
        m_cross_table.seek(0);
        stream.w(m_cross_table.pointer(), m_cross_table.size());
        m_cross_table.clear();
    }

    u32 dwfGetEdgeCount()
    {
        u32 l_dwResult = 0;
        for (auto &i : m_tpVertices)
            l_dwResult += i.tNeighbourCount;
        return (l_dwResult);
    }

    u32 dwfGetDeathPointCount()
    {
        u32 l_dwResult = 0;
        for (auto &i : m_tpVertices)
            l_dwResult += i.tDeathPointCount;
        return (l_dwResult);
    }

    void vfGenerateDeathPoints(
        int iGraphIndex, CGameLevelCrossTable* tpCrossTable, CLevelGraph* tpAI_Map, u32& dwDeathPointCount)
    {
        xr_vector<u32> l_dwaNodes;
        l_dwaNodes.clear();
        {
            for (u32 i = 0, n = tpCrossTable->m_tCrossTableHeader.dwNodeCount; i < n; i++)
                if (tpCrossTable->m_tpaCrossTable[i].tGraphIndex == iGraphIndex)
                    l_dwaNodes.push_back(i);
        }

        R_ASSERT2(!l_dwaNodes.empty(), "Can't create at least one death point for specified graph point");

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(l_dwaNodes.begin(), l_dwaNodes.end(), g);

        u32 m = l_dwaNodes.size() > 10 ? std::min(iFloor(.1f * l_dwaNodes.size()), 255) : l_dwaNodes.size(),
            l_dwStartIndex = m_tpLevelPoints.size();
        m_tpLevelPoints.resize(l_dwStartIndex + m);
        auto I = m_tpLevelPoints.begin() + l_dwStartIndex;
        auto E = m_tpLevelPoints.end();
        auto i = l_dwaNodes.begin();

        dwDeathPointCount = m;

        for (; I != E; ++I, ++i)
        {
            (*I).tNodeID = *i;
            (*I).tPoint = tpAI_Map->vertex_position(*i);
            (*I).fDistance = tpCrossTable->vertex(*i).distance();
        }
    }
};

class CGraphMerger
{
public:
    CGraphMerger(LPCSTR game_graph_id, LPCSTR name, bool rebuild);
};

void read_levels(CInifile* Ini, xr_set<CLevelInfo>& levels, bool rebuild_graph, xr_vector<LPCSTR>* needed_levels)
{
    LPCSTR _N, V;
    string_path caFileName, file_name;
    for (u32 k = 0; Ini->r_line("levels", k, &_N, &V); k++)
    {
        string256 N;
        xr_strcpy(N, _N);
        xr_strlwr(N);

        if (!Ini->section_exist(N))
        {
            Msg("! There is no section %s in the %s!", N, GAME_CONFIG);
            continue;
        }

        if (!Ini->line_exist(N, "name"))
        {
            Msg("! There is no line \"name\" in the section %s!", N);
            continue;
        }

        if (!Ini->line_exist(N, "id"))
        {
            Msg("! There is no line \"id\" in the section %s!", N);
            continue;
        }

        if (!Ini->line_exist(N, "offset"))
        {
            Msg("! There is no line \"offset\" in the section %s!", N);
            continue;
        }

        u8 id = Ini->r_u8(N, "id");
        auto _S = Ini->r_string(N, "name");
        string256 S;
        xr_strcpy(S, _S);
        xr_strlwr(S);

        if (needed_levels)
        {
            bool found = false;
            for (const auto &i : *needed_levels)
            {
                if (!xr_strcmp(i, S))
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                continue;
        }

        {
            bool ok = true;
            for (auto &i : levels)
            {
                if (!xr_strcmp(i.m_section, N))
                {
                    Msg("! Duplicated line %s in section \"levels\" in the %s", N, GAME_CONFIG);
                    ok = false;
                    break;
                }
                if (!xr_strcmp(i.m_name, S))
                {
                    Msg("! Duplicated level name %s in the %s, sections %s, %s", S, GAME_CONFIG, *i.m_section, N);
                    ok = false;
                    break;
                }
                if (i.m_id == id)
                {
                    Msg("! Duplicated level id %d in the %s, section %s, level %s", id, GAME_CONFIG, N, S);
                    ok = false;
                    break;
                }
            }
            if (!ok)
                continue;
        }
        IReader* reader;
        // ai
        strconcat(sizeof(caFileName), caFileName, S, "\\", LEVEL_GRAPH_NAME);
        FS.update_path(file_name, "$game_levels$", caFileName);
        if (!FS.exist(file_name))
        {
            Msg("! There is no ai-map for the level %s! (level is not included into the game graph)", S);
            continue;
        }

        {
            reader = FS.r_open(file_name);
            CLevelGraph::CHeader header;
            reader->r(&header, sizeof(header));
            FS.r_close(reader);
            if (header.version() != XRAI_CURRENT_VERSION)
            {
                Msg("! AI-map for the level %s is incompatible (version mismatch)! (level is not included into the game graph)", S);
                continue;
            }
        }

        levels.insert(CLevelInfo(id, S, Ini->r_fvector3(N, "offset"), N));
    }
}

LPCSTR generate_temp_file_name(LPCSTR header0, LPCSTR header1, string_path& buffer)
{
    string_path path;
    FS.update_path(path, "$app_data_root$", "temp");
    xr_strcat(path, sizeof(path), "\\");

    _mkdir(path);

    strconcat(sizeof(buffer), buffer, path, header0, header1);
    return (buffer);
}

void fill_needed_levels(pstr levels, xr_vector<LPCSTR>& result)
{
    auto I = levels;
    for (auto J = I;; ++I)
    {
        if (*I != ',')
        {
            if (*I)
                continue;

            result.push_back(J);
            break;
        }

        *I = 0;
        result.push_back(J);
        J = I + 1;
    }
}

CGraphMerger::CGraphMerger(LPCSTR game_graph_id, LPCSTR name, bool rebuild)
{
    // load all the graphs
    Logger.Phase("Processing level graphs");

    CInifile* Ini = xr_new<CInifile>(INI_FILE);
    R_ASSERT(Ini->section_exist("levels"));

    tGraphHeader.m_guid = generate_guid();

    GRAPH_P_MAP tpGraphs;
    string4096 S1, S2;
    CGameGraph::SLevel tLevel;
    u32 dwOffset = 0;
    u32 l_dwPointOffset = 0;
    LEVEL_POINT_STORAGE l_tpLevelPoints;
    l_tpLevelPoints.clear();

    xr_set<CLevelInfo> levels;
    xr_vector<LPCSTR> needed_levels;
    string4096 levels_string;
    xr_strcpy(levels_string, name);
    xr_strlwr(levels_string);
    fill_needed_levels(levels_string, needed_levels);

    read_levels(Ini, levels, rebuild, &needed_levels);

    for (const auto &i : levels)
    {
        tLevel.m_offset = i.m_offset;
        tLevel.m_name = i.m_name;
        xr_strcpy(S1, sizeof(S1), *i.m_name);
        strconcat(sizeof(S2), S2, name, S1);
        strconcat(sizeof(S1), S1, S2, "\\");
        tLevel.m_id = i.m_id;
        tLevel.m_section = i.m_section;
        Msg("%9s %2d %s", "level", tLevel.id(), *tLevel.m_name);
        string_path _0, _1;
        generate_temp_file_name("local_graph_", *tLevel.m_name, _0);
        generate_temp_file_name("raw_cross_table_", *tLevel.m_name, _1);
        string_path level_folder;
        FS.update_path(level_folder, "$game_levels$", *tLevel.m_name);
        xr_strcat(level_folder, "\\");
        CGameGraphBuilder().build_graph(_0, _1, level_folder);
        auto tpLevelGraph = xr_new<::CLevelGameGraph>(_0, _1, &tLevel, level_folder, dwOffset, tLevel.id(), Ini);
        dwOffset += tpLevelGraph->m_tpGraph->header().vertex_count();
        R_ASSERT2(tpGraphs.find(tLevel.id()) == tpGraphs.end(), "Level ids _MUST_ be different!");
        tpGraphs.insert(std::make_pair(tLevel.id(), tpLevelGraph));
        tGraphHeader.m_levels.insert(std::make_pair(tLevel.id(), tLevel));
    }

    R_ASSERT(tpGraphs.size());

    Logger.Phase("Adding interconnection points");
    {
        for (auto &i : tpGraphs)
        {
            for (auto &j : i.second->m_tVertexMap)
            {
                if (j.second.caConnectName[0])
                {
                    CGameGraph::CEdge tGraphEdge;
                    auto& tConnectionVertex = j.second;
                    auto K = tpGraphs.find(tConnectionVertex.dwLevelID);
                    if (K == tpGraphs.end())
                    {
                        Msg("Cannot find level with level_id %d. Connection point will not be generated!", tConnectionVertex.dwLevelID);
                        continue;
                    }
                    R_ASSERT(K != tpGraphs.end());
                    auto M = (*K).second->m_tVertexMap.find(tConnectionVertex.caConnectName);
                    if (M == (*K).second->m_tVertexMap.end())
                    {
                        Msg("Level %s with id %d has an INVALID connection point %s,\nwhich references to graph point %s on the level %s with id %d\n",
                            *i.second->m_tLevel.name(), i.second->m_tLevel.id(), j.first,
                            tConnectionVertex.caConnectName, *(*K).second->m_tLevel.name(), (*K).second->m_tLevel.id());
                        R_ASSERT(M != (*K).second->m_tVertexMap.end());
                    }

                    //if (!xr_stricmp("l06_rostok",*i.second->m_tLevel.name()))
                    //    __asm int 3;

                    Msg("Level %s with id %d has VALID connection point %s,\nwhich references to graph point %s on the level %s with id %d\n",
                        *i.second->m_tLevel.name(), i.second->m_tLevel.id(), j.first,
                        tConnectionVertex.caConnectName, *(*K).second->m_tLevel.name(), (*K).second->m_tLevel.id());

                    VERIFY(((*M).second.tGraphID + (*K).second->m_dwOffset) <
                        (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
                    tGraphEdge.m_vertex_id = (GameGraph::_GRAPH_ID)((*M).second.tGraphID + (*K).second->m_dwOffset);
                    VERIFY3(tConnectionVertex.tGraphID < i.second->m_tpVertices.size(), "Rebuild graph for the level", *i.second->m_tLevel.name());
                    VERIFY3((*M).second.tGraphID < (*K).second->m_tpVertices.size(), "Rebuild graph for the level", *(*K).second->m_tLevel.name());
                    tGraphEdge.m_path_distance =
                        i.second->m_tpVertices[tConnectionVertex.tGraphID].tGlobalPoint.distance_to(
                        (*K).second->m_tpVertices[(*M).second.tGraphID].tGlobalPoint);
                    i.second->vfAddEdge(j.second.tGraphID, tGraphEdge);
                    //tGraphEdge.dwVertexNumber	= j.second.tGraphID + i.second->m_dwOffset;
                    //(*K).second->vfAddEdge((*M).second.tGraphID,tGraphEdge);
                }
            }
        }
    }
    // counting edges
    {
        tGraphHeader.m_edge_count = 0;
        tGraphHeader.m_death_point_count = 0;
        for (auto &i : tpGraphs)
        {
            VERIFY((u32(tGraphHeader.m_edge_count) + i.second->dwfGetEdgeCount()) < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
            tGraphHeader.m_edge_count += (GameGraph::_GRAPH_ID)i.second->dwfGetEdgeCount();
            tGraphHeader.m_death_point_count += i.second->dwfGetDeathPointCount();
        }
    }

    ///////////////////////////////////////////////////

    // save all the graphs
    Logger.Phase("Saving graph being merged");
    CMemoryWriter F;
    tGraphHeader.m_version = XRAI_CURRENT_VERSION;
    VERIFY(dwOffset < (u32(1) << (8 * sizeof(GameGraph::_GRAPH_ID))));
    tGraphHeader.m_vertex_count = (GameGraph::_GRAPH_ID)dwOffset;
    tGraphHeader.save(&F);

    u32 vertex_count = 0;
    dwOffset *= sizeof(CGameGraph::CGameVertex);
    u32 l_dwOffset = F.size();
    l_dwPointOffset = dwOffset + tGraphHeader.edge_count() * sizeof(CGameGraph::CEdge);
    u32 l_dwStartPointOffset = l_dwPointOffset;

    for (auto &i : tpGraphs)
    {
        i.second->vfSaveVertices(F, dwOffset, l_dwPointOffset, &l_tpLevelPoints);
        vertex_count += i.second->m_tpGraph->header().vertex_count();
    }

    for (auto &i : tpGraphs)
        i.second->vfSaveEdges(F);

    l_tpLevelPoints.clear();
    for (auto &i : tpGraphs)
        l_tpLevelPoints.insert(l_tpLevelPoints.end(), i.second->m_tpLevelPoints.begin(), i.second->m_tpLevelPoints.end());

    R_ASSERT2(l_dwStartPointOffset == F.size() - l_dwOffset, "Graph file format is corrupted");

    for (auto &i : l_tpLevelPoints)
        save_data(i, F);

    for (auto &i : tpGraphs)
    {
        Msg("cross_table offset: %d", F.size());
        i.second->save_cross_table(F);
    }

    string256 l_caFileName;
    xr_strcpy(l_caFileName, game_graph_id);
    F.save_to(l_caFileName);

    // free all the graphs
    Logger.Phase("Freeing resources being allocated");

    for (auto &i : tpGraphs)
        xr_free(i.second);

    xr_delete(Ini);
}

void xrMergeGraphs(LPCSTR game_graph_id, LPCSTR name, bool rebuild) { CGraphMerger A(game_graph_id, name, rebuild); }
