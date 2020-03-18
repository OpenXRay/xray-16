#pragma once
class XRayWallMarkArray:public IWallMarkArray 
{
public:
	XRayWallMarkArray();
	virtual void Copy(IWallMarkArray &_in) ;

	virtual void	AppendMark(LPCSTR s_textures);
	virtual void	clear();
	virtual bool	empty() ;
	virtual wm_shader GenerateWallmark();
};
