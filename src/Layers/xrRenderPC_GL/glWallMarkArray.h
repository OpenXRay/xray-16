#pragma once

#include "..\..\Include\xrRender\WallMarkArray.h"

class glWallMarkArray :
	public IWallMarkArray
{
public:
	virtual ~glWallMarkArray();
	virtual void Copy(IWallMarkArray &_in);

	virtual void AppendMark(LPCSTR s_textures);
	virtual void clear();
	virtual bool empty();
	virtual wm_shader GenerateWallmark();

	ref_shader*	glGenerateWallmark();
private:
	using ShaderVec = xr_vector<ref_shader>;
	using ShaderIt = ShaderVec::iterator;

	ShaderVec	m_CollideMarks;
};
