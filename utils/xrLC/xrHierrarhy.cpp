#include "stdafx.h"
#include "build.h"
#include "OGF_Face.h"

void CBuild::BuildHierrarhy()
{
	Fvector		scene_size;
	float		delimiter;

	scene_bb.getsize(scene_size);
	delimiter = _MAX(scene_size.x,_MAX(scene_size.y,scene_size.z));
	delimiter *= 2;

	int		iLevel = 1;
	float	SizeLimit = g_params.m_maxsize/8;
	if		(SizeLimit<4.f) SizeLimit=4.f;
	for (; SizeLimit<=delimiter; SizeLimit*=2)
	{
		int iSize			= g_tree.size();
		Status("Level #%d",iLevel);

		for (int I=0; I<iSize; I++)
		{
			if (g_tree[I]->bConnected) continue;
			OGF_Node* pNode = xr_new<OGF_Node> (iLevel);
			pNode->AddChield(I);

			// Find best object to connect with
			for (int J=0; J<iSize; J++)
			{
				OGF_Base* candidate = g_tree[J];
				if ( candidate->bConnected) continue;

				Fbox		bb;	
				Fvector		size;
				bb.set		(pNode->bbox);
				bb.merge	(candidate->bbox);
				bb.getsize	(size);
				if (size.x>SizeLimit || size.y>SizeLimit || size.z>SizeLimit) continue;
				pNode->AddChield(J);
			}

			if (pNode->chields.size()>1) {
				pNode->CalcBounds();
				g_tree.push_back(pNode);
			} else {
				g_tree[I]->bConnected = false;
				xr_delete pNode;
			}
			Progress(float(I)/float(iSize));
		}
		
		clMsg("#%2d [%3.1f]: %d nodes",iLevel,SizeLimit,g_tree.size()-iSize);
		if (iSize != g_tree.size()) iLevel++;
	}
	g_TREE_ROOT = g_tree.back();
	clMsg("* TREE levels %d, TREE size %d",iLevel,g_tree.size());
}
