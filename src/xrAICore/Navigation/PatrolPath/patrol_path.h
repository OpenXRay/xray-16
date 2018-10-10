////////////////////////////////////////////////////////////////////////////
//	Module 		: patrol_path.h
//	Created 	: 15.06.2004
//  Modified 	: 15.06.2004
//	Author		: Dmitriy Iassenev
//	Description : Patrol path
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrAICore/Navigation/graph_abstract.h"
#include "xrAICore/Navigation/PatrolPath/patrol_point.h"

enum EPatrolStartType : u32
{
    ePatrolStartTypeFirst = u32(0),
    ePatrolStartTypeLast,
    ePatrolStartTypeNearest,
    ePatrolStartTypePoint,
    ePatrolStartTypeNext,
    ePatrolStartTypeDummy = u32(-1),
};
enum EPatrolRouteType : u32
{
    ePatrolRouteTypeStop = u32(0),
    ePatrolRouteTypeContinue,
    ePatrolRouteTypeDummy = u32(-1),
};

class XRAICORE_API CPatrolPath : public CGraphAbstractSerialize<CPatrolPoint, float, u32>
{
private:
    struct CAlwaysTrueEvaluator
    {
        IC bool operator()(const Fvector& position) const { return (true); }
    };

protected:
    typedef CGraphAbstractSerialize<CPatrolPoint, float, u32> inherited;

public:
#ifdef DEBUG
    shared_str m_name;
#endif

public:
    CPatrolPath(shared_str name = "");
    virtual ~CPatrolPath();
    CPatrolPath& load_raw(const CLevelGraph* level_graph, const CGameLevelCrossTable* cross,
        const CGameGraph* game_graph, IReader& stream);
    IC const CVertex* point(shared_str name) const;
    template <typename T>
    IC const CVertex* point(const Fvector& position, const T& evaluator) const;
    IC const CVertex* point(const Fvector& position) const;

#ifdef DEBUG
public:
    virtual void load(IReader& stream);
    IC void name(const shared_str& name);
#endif
};

#include "xrAICore/Navigation/PatrolPath/patrol_path_inline.h"
