////////////////////////////////////////////////////////////////////////////
//	Module 		: alife_movement_manager_holder.h
//	Created 	: 24.07.2009
//  Modified 	: 24.07.2009
//	Author		: Plichko Alexander
//	Description : movement manager holder
////////////////////////////////////////////////////////////////////////////
#pragma once

class CMovementManagerHolder
{
public:
    GameGraph::_GRAPH_ID m_tNextGraphID;
    GameGraph::_GRAPH_ID m_tPrevGraphID;
    float m_fGoingSpeed;
    float m_fCurrentLevelGoingSpeed;
    float m_fCurSpeed;
    float m_fDistanceFromPoint;
    float m_fDistanceToPoint;
    GameGraph::TERRAIN_VECTOR m_tpaTerrain;

#ifdef XRGAME_EXPORTS
public:
    virtual void on_location_change() const = 0;
    virtual CSE_ALifeDynamicObject const& get_object() const = 0;
    virtual CSE_ALifeDynamicObject& get_object() = 0;
#endif //#ifdef XRGAME_EXPORTS
}; // CMovementManagerHolder
