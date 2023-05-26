#include "stdafx.h"
#include "Blender.h"
#include "blender_clsid.h"

IC bool p_sort(IBlender *A, IBlender *B)
{
	return stricmp(A->getComment(), B->getComment()) < 0;
}

#define TYPES_EQUAL(A, B) (typeid(A).raw_name() == typeid(B).raw_name())

void IBlender::CreatePalette(xr_vector<IBlender *> &palette)
{
	// Create palette itself
	R_ASSERT(palette.empty());
	palette.push_back(Create(B_DEFAULT));
	palette.push_back(Create(B_DEFAULT_AREF));
	palette.push_back(Create(B_VERT));
	palette.push_back(Create(B_VERT_AREF));
	palette.push_back(Create(B_SCREEN_SET));
	palette.push_back(Create(B_SCREEN_GRAY));
	palette.push_back(Create(B_EDITOR_WIRE));
	palette.push_back(Create(B_EDITOR_SEL));
	palette.push_back(Create(B_LIGHT));
	palette.push_back(Create(B_LaEmB));
	palette.push_back(Create(B_LmEbB));
	palette.push_back(Create(B_BmmD));
	palette.push_back(Create(B_B));
	palette.push_back(Create(B_SHADOW_WORLD));
	palette.push_back(Create(B_BLUR));
	palette.push_back(Create(B_MODEL));
	palette.push_back(Create(B_MODEL_EbB));
	palette.push_back(Create(B_DETAIL));
	palette.push_back(Create(B_TREE));
	palette.push_back(Create(B_PARTICLE));

	// Remove duplicated classes (some of them are really the same in different renderers)
	for (u32 i = 0; i < palette.size(); i++)
	{
		IBlender *A = palette[i];
		for (u32 j = i + 1; j < palette.size(); j++)
		{
			IBlender *B = palette[j];
			if ((A && B) && TYPES_EQUAL(*A, *B))
			{
				xr_delete(palette[j]);
				j--;
			}
		}
	}
	for (u32 i = palette.size(); i != 0; i--)
	{
		u32 real_i = i - 1;
		if (!palette[real_i])
			palette.erase(palette.begin() + real_i);
	}
	// Sort by desc and return
	std::sort(palette.begin(), palette.end(), p_sort);
}
