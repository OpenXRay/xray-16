#include "stdafx.h"
#include "glWallMarkArray.h"

#include "glUIShader.h"

void glWallMarkArray::Copy(IWallMarkArray &_in)
{
	*this = *(glWallMarkArray*)&_in;
}

glWallMarkArray::~glWallMarkArray()
{
	for (ShaderIt it = m_CollideMarks.begin(); it != m_CollideMarks.end(); ++it)
		it->destroy();
}

void glWallMarkArray::AppendMark(LPCSTR s_textures)
{
	ref_shader	s;
	s.create("effects\\wallmark", s_textures);
	m_CollideMarks.push_back(s);
}

void glWallMarkArray::clear()
{
	return m_CollideMarks.clear();
}

bool glWallMarkArray::empty()
{
	return m_CollideMarks.empty();
}

wm_shader glWallMarkArray::GenerateWallmark()
{
	wm_shader	res;
	if (!m_CollideMarks.empty())
		((glUIShader*)&*res)->hShader = m_CollideMarks[::Random.randI(0, m_CollideMarks.size())];
	return res;
}

ref_shader*	glWallMarkArray::glGenerateWallmark()
{
	return m_CollideMarks.empty() ? NULL :
		&m_CollideMarks[::Random.randI(0, m_CollideMarks.size())];
}
