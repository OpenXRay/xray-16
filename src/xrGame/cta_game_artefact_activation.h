////////////////////////////////////////////////////////////////////////////////
//	Module		:	cta_game_artefact.h
//	Created		:	19.12.2007
//	Modified	:	19.12.2007
//	Autor		:	Alexander Maniluk
//	Description	:	Artefact object for Capture The Artefact game mode
////////////////////////////////////////////////////////////////////////////////
#ifndef CTA_GAME_ARTEFACT_ACTIVATION
#define CTA_GAME_ARTEFACT_ACTIVATION

#include "artefact_activation.h"

class	CtaArtefactActivation :
		public SArtefactActivation
{
public:
							CtaArtefactActivation	(CArtefact* af, u32 owner_id);
	virtual					~CtaArtefactActivation	();

	virtual		void		UpdateActivation		();
	virtual		void		Load					();
	virtual		void		Start					();
	virtual		void		Stop					();
	virtual		void		ChangeEffects			();
	virtual		void		UpdateEffects			();
	virtual		void		SpawnAnomaly			();
	virtual		void		PhDataUpdate			(float step);
private:
	typedef		SArtefactActivation		inherited;
}; // class CtaArtefactActivation
		

#endif // CTA_GAME_ARTEFACT_ACTIVATION
