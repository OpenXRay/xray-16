#ifndef	ColorMapManager_included
#define	ColorMapManager_included
#pragma once

//	Reduces amount of work if the texture was not changed.
//	Stores used textures in a separate map to avoid removal of
//	of color map textures from memory.

class ColorMapManager
{
public:
	ColorMapManager();

	void	SetTextures(const shared_str &tex0, const shared_str &tex1);

private:

	void	UpdateTexture(const shared_str &strTexName, int iTex);

private:

	struct str_pred : public std::binary_function<const shared_str&, const shared_str &, bool>	
	{
		IC bool operator()(const shared_str &x, const shared_str &y) const
		{	return x<y;	}
	};

	DEFINE_MAP_PRED(shared_str, ref_texture, map_Tex, map_TexIt, str_pred);

private:

	ref_texture		m_CMap[2];
	shared_str		m_strCMap[2];

	map_Tex			m_TexCache;
};

#endif	//	ColorMapManager_included