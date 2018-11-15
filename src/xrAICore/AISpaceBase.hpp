#pragma once

#include "xrCore/xrCore.h"

class CGameGraph;
class CGameLevelCrossTable;
class CLevelGraph;
class CGraphEngine;
class CPatrolPathStorage;

class XRAICORE_API AISpaceBase
{
protected:
    CGameGraph* m_game_graph = nullptr; // not owned by AISpaceBase
    CLevelGraph* m_level_graph = nullptr;
    CGraphEngine* m_graph_engine = nullptr;
    CPatrolPathStorage* m_patrol_path_storage = nullptr;

protected:
    AISpaceBase();
    void Load(const char* levelName);
    void Unload(bool reload);
    void Initialize();
    void Validate(u32 levelId) const;
    void patrol_path_storage_raw(IReader& stream);
    void patrol_path_storage(IReader& stream);
    void SetGameGraph(CGameGraph* gameGraph);

public:
    virtual ~AISpaceBase();
    inline CGameGraph& game_graph() const;
    inline CGameGraph* get_game_graph() const;
    inline CLevelGraph& level_graph() const;
    inline const CLevelGraph* get_level_graph() const;
    const CGameLevelCrossTable& cross_table() const;
    const CGameLevelCrossTable* get_cross_table() const;
    inline const CPatrolPathStorage& patrol_paths() const;
    inline CGraphEngine& graph_engine() const;
};

inline CGameGraph& AISpaceBase::game_graph() const
{
    VERIFY(m_game_graph);
    return *m_game_graph;
}

inline CGameGraph* AISpaceBase::get_game_graph() const { return m_game_graph; }
inline CLevelGraph& AISpaceBase::level_graph() const
{
    VERIFY(m_level_graph);
    return *m_level_graph;
}

inline const CLevelGraph* AISpaceBase::get_level_graph() const { return m_level_graph; }
inline CGraphEngine& AISpaceBase::graph_engine() const
{
    VERIFY(m_graph_engine);
    return *m_graph_engine;
}

inline const CPatrolPathStorage& AISpaceBase::patrol_paths() const
{
    VERIFY(m_patrol_path_storage);
    return *m_patrol_path_storage;
}
