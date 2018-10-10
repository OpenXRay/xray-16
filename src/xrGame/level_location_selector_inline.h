////////////////////////////////////////////////////////////////////////////
//	Module 		: level_location_selector_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 18.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Level location selector inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _VertexEvaluator, typename _vertex_id_type\
>

#define CLevelLocationSelector CBaseLocationSelector<CLevelGraph, _VertexEvaluator, _vertex_id_type>

TEMPLATE_SPECIALIZATION
IC CLevelLocationSelector::CBaseLocationSelector(CRestrictedObject* object) : inherited(object) {}
TEMPLATE_SPECIALIZATION
IC void CLevelLocationSelector::before_search(_vertex_id_type& vertex_id)
{
    if (m_restricted_object)
    {
        if (!m_restricted_object->accessible(vertex_id))
        {
            Fvector dest_pos;
            vertex_id = m_restricted_object->accessible_nearest(m_graph->vertex_position(vertex_id), dest_pos);
        }
        m_restricted_object->add_border(vertex_id, m_evaluator->m_fRadius);
    }
}

TEMPLATE_SPECIALIZATION
IC void CLevelLocationSelector::after_search()
{
    if (m_restricted_object)
        m_restricted_object->remove_border();
}

#undef TEMPLATE_SPECIALIZATION
#undef CLevelLocationSelector
