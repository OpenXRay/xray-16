#ifndef dxWallMarkArray_included
#define dxWallMarkArray_included
#pragma once

#include "..\..\Include\xrRender\WallMarkArray.h"

class dxWallMarkArray : public IWallMarkArray
{
public:
	virtual ~dxWallMarkArray();
	virtual void Copy(IWallMarkArray &_in);

	virtual void AppendMark(LPCSTR s_textures);
	virtual void clear();
	virtual bool empty();
	virtual wm_shader GenerateWallmark();

	ref_shader*	dxGenerateWallmark();
private:
	DEFINE_VECTOR(ref_shader,ShaderVec,ShaderIt);

	ShaderVec	m_CollideMarks;
};

#endif	//	WallMarkArray_included