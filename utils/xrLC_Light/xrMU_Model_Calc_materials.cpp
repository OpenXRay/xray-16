#include "stdafx.h"
#include "xrMU_Model.h"
#include "../shader_xrlc.h"

bool	cmp_face_material		(_face* f1, _face* f2)
{
	return f1->dwMaterial < f2->dwMaterial;
}
//static xrMU_Model::v_faces temp_vector;
void xrMU_Model::calc_materials	()
{
	
	xrMU_Model::v_faces &temp_vector			= m_faces;

	std::sort			(temp_vector.begin(),temp_vector.end(),cmp_face_material);

	_subdiv				current;
	current.material	= temp_vector[0]->dwMaterial;
	current.start		= 0;
	current.count		= 1;

	for (u32 it=1; it<temp_vector.size(); it++)
	{
		if (current.material != temp_vector[it]->dwMaterial)
		{
			// end of strip 
			m_subdivs.push_back	(current);
			current.material	= temp_vector[it]->dwMaterial;
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
		_face*		first	= temp_vector[m_subdivs[it].start];
		if (first->Shader().flags.bRendering)	continue;

		m_subdivs.erase	(m_subdivs.begin()+it);
		it--;
	}

	clMsg	("model '%s' - %d subdivisions",*m_name,m_subdivs.size());
}
