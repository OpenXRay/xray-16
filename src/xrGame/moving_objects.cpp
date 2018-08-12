////////////////////////////////////////////////////////////////////////////
//	Module 		: moving_objects.cpp
//	Created 	: 27.03.2007
//  Modified 	: 27.03.2007
//	Author		: Dmitriy Iassenev
//	Description : moving objects
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "moving_objects.h"
#include "ai_space.h"
#include "xrAICore/Navigation/level_graph.h"
#include "moving_object.h"

moving_objects::moving_objects() : m_tree(0) {}
moving_objects::~moving_objects() { xr_delete(m_tree); }
void moving_objects::on_level_load()
{
    xr_delete(m_tree);
    m_tree = new TREE(
        ai().level_graph().header().box(), ai().level_graph().header().cell_size() * .5f, 16 * 1024, 16 * 1024);
}

void moving_objects::register_object(moving_object* moving_object)
{
    VERIFY2(m_objects.find(moving_object) == m_objects.end(),
        make_string("moving object %s is registers twice", *moving_object->id()));

#ifdef DEBUG
    m_objects.insert(moving_object);
#endif // DEBUG

    VERIFY(m_tree);
    m_tree->insert(moving_object);
}

void moving_objects::unregister_object(moving_object* moving_object)
{
    VERIFY2(m_objects.find(moving_object) != m_objects.end(),
        make_string("moving object %s is not yet registered or unregisters twice", *moving_object->id()));

#ifdef DEBUG
    m_objects.erase(m_objects.find(moving_object));
#endif // DEBUG

    VERIFY(m_tree);
    m_tree->remove(moving_object);
}

void moving_objects::on_object_move(moving_object* moving_object)
{
    VERIFY2(m_objects.find(moving_object) != m_objects.end(),
        make_string("moving object %s is not yet registered", *moving_object->id()));

#pragma todo("this place can be optimized in case of slowdowns")
    VERIFY(m_tree);

    m_tree->remove(moving_object);

    moving_object->update_position();

    m_tree->insert(moving_object);
}

void moving_objects::clear() { m_previous_collisions.clear(); }
