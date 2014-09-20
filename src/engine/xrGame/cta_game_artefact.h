////////////////////////////////////////////////////////////////////////////////
//	Module		:	cta_game_artefact.h
//	Created		:	19.12.2007
//	Modified	:	19.12.2007
//	Autor		:	Alexander Maniluk
//	Description	:	Artefact object for Capture The Artefact game mode
////////////////////////////////////////////////////////////////////////////////

#ifndef CTA_GAME_ARTEFACT
#define CTA_GAME_ARTEFACT

#include "Artefact.h"
#include "game_base.h"

class game_cl_CaptureTheArtefact;
class	CtaGameArtefact :
		public	CArtefact
{
public:
						CtaGameArtefact				();
	virtual				~CtaGameArtefact			();

	virtual		bool	Action						(s32 cmd, u32 flags);
	virtual		void	OnStateSwitch				(u32 S);
	virtual		void	OnAnimationEnd				(u32 state);
	virtual		void	UpdateCLChild				();
	virtual		bool	CanTake						() const;
	//virtual		void	net_Export					(NET_Packet& P);
	virtual void				PH_A_CrPr			();
	//virtual void				Interpolate			();
protected:
	virtual		void	CreateArtefactActivation	();
	virtual		void	InitializeArtefactRPoint	();
	//virtual		BOOL	net_Relevant				();
private:
				bool	IsMyTeamArtefact			();
	typedef CArtefact inherited;
	game_cl_CaptureTheArtefact*			m_game;
	Fvector3 const *					m_artefact_rpoint;
	ETeam								m_my_team;
}; //class CtaGameArtefact

#endif //CTA_GAME_ARTEFACT