#pragma once
#ifdef DEBUG
#include "xrCore/xrCore.h"
#include "Include/xrRender/DebugShader.h"
class CGameGraph;
class CLevelGraph;
class CCoverPoint;

class LevelGraphDebugRender
{
private:
    CGameGraph* gameGraph;
    CLevelGraph* levelGraph;
    debug_shader debugShader;
    int currentLevelId;
    bool currentActual;
    Fvector currentCenter;
    Fvector currentRadius;
    xr_vector<CCoverPoint*> coverPointCache;

private:
    Fvector ConvertPosition(const Fvector& pos);
    void DrawEdge(int vid1, int vid2);
    void DrawVertex(int vid);
    void DrawStalkers(int vid);
    void DrawObjects(int vid);
    void UpdateCurrentInfo();
    void DrawNodes();
    void DrawRestrictions();
    void DrawCovers();
    void DrawGameGraph();
    void DrawObjects();
    void DrawDebugNode();
    void Modify(int vid, Fbox& bbox);

public:
    LevelGraphDebugRender();
    ~LevelGraphDebugRender();
    void SetupCurrentLevel(int levelId);
    void Render(CGameGraph& gameGraph, CLevelGraph& levelGraph);
};

#endif // DEBUG
