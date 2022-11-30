////////////////////////////////////////////////////////////////////////////
//	Module 		: game_path_manager_inline.h
//	Created 	: 02.10.2001
//  Modified 	: 12.11.2003
//	Author		: Dmitriy Iassenev
//	Description : Game path manager inline functions
////////////////////////////////////////////////////////////////////////////

#pragma once

#define TEMPLATE_SPECIALIZATION \
    template <typename _VertexEvaluator, typename _vertex_id_type, typename _index_type>

#define CGameManagerTemplate CBaseGamePathManager<_VertexEvaluator, _vertex_id_type, _index_type>

TEMPLATE_SPECIALIZATION
IC CGameManagerTemplate::CBaseGamePathManager(CRestrictedObject* object) : inherited(object) {}
TEMPLATE_SPECIALIZATION
IC void CGameManagerTemplate::reinit(const CGameGraph* graph) { inherited::reinit(graph); }
TEMPLATE_SPECIALIZATION
IC bool CGameManagerTemplate::actual() const
{
    return (inherited::actual(this->m_object->object().ai_location().game_vertex_id(), this->dest_vertex_id()));
}

TEMPLATE_SPECIALIZATION
IC void CGameManagerTemplate::before_search(const _vertex_id_type start_vertex_id, const _vertex_id_type dest_vertex_id)
{
}

TEMPLATE_SPECIALIZATION
IC void CGameManagerTemplate::after_search() {}
TEMPLATE_SPECIALIZATION
IC bool CGameManagerTemplate::completed() const
{
    if (this->path().empty() || (this->m_intermediate_index >= (_vertex_id_type)this->path().size() - 1))
        return (inherited::completed());
    return false;
}

TEMPLATE_SPECIALIZATION
IC void CGameManagerTemplate::select_intermediate_vertex()
{
    VERIFY(!this->path().empty());
    if (this->m_intermediate_index != _index_type(-1))
        ++this->m_intermediate_index;
    else if (this->path().size() < 2)
        this->m_intermediate_index = 0;
    else
        this->m_intermediate_index = 1;
}

#undef TEMPLATE_SPECIALIZATION
#undef CGameManagerTemplate
