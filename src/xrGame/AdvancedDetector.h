#pragma once
#include "CustomDetector.h"
class CUIArtefactDetectorAdv;

class CAdvancedDetector :public CCustomDetector
{
	typedef CCustomDetector	inherited;
public:
					CAdvancedDetector			();
	virtual			~CAdvancedDetector			();
	virtual void	on_a_hud_attach				();
	virtual void	on_b_hud_detach				();
protected:
	virtual void 	UpdateAf					();
	virtual void 	CreateUI					();
	CUIArtefactDetectorAdv& ui					();

};

//	static void _BCL		BoneCallback					(CBoneInstance *B);
