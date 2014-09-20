#include "stdafx.h"
#include "xrMU_Model.h"
#include "../shader_xrlc.h"

bool	cmp_face_material		(xrMU_Model::_face* f1, xrMU_Model::_face* f2)
{
	return f1->dwMaterial < f2->dwMaterial;
}

void xrMU_Model::calc_materials	()
{
	std::sort			(m_faces.begin(),m_faces.end(),cmp_face_material);

	_subdiv				current;
	current.material	= m_faces[0]->dwMaterial;
	current.start		= 0;
	current.count		= 1;

	for (u32 it=1; it<m_faces.size(); it++)
	{
		if (current.material != m_faces[it]->dwMaterial)
		{
			// end of strip 
			m_subdivs.push_back	(current);
			current.material	= m_faces[it]->dwMaterial;
			current.start		= it;
			current.count		= 1;
		} 
		else 
		{	
			// strip continues to grow
			current.count	++;
		}
	}
	m_subdivs.push_back	(current);

	// remove non-visible materials
	for (s32 it=0; it<s32(m_subdivs.size()); it++)
	{
		_face*		first	= m_faces[m_subdivs[it].start];
		if (first->Shader().flags.bRendering)	continue;

		m_subdivs.erase	(m_subdivs.begin()+it);
		it--;
	}

	clMsg	("model '%s' - %d subdivisions",*m_name,m_subdivs.size());
}
