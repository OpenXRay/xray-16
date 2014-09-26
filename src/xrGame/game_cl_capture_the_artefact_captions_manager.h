#ifndef GAME_CL_CTA_CAPTIONS_MANAGER
#define GAME_CL_CTA_CAPTIONS_MANAGER

#include <boost/noncopyable.hpp>

class game_cl_CaptureTheArtefact;
class CUIGameCTA;

class	CTAGameClCaptionsManager : 
		private boost::noncopyable
{
private:
			bool							m_press_fire2spect_showed;
			bool							m_press_jump2payspaw_showed;
			bool							m_can_show_payspawn;
			bool							m_can_show_buy;
			bool							m_can_spawn;
			game_cl_CaptureTheArtefact*		parent_game_object;
			CUIGameCTA*						parent_game_ui;
			ETeam							m_winner_team;
	void	ShowInProgressCaptions			();
	void	ShowPendingCaptions				();
	void	ShowScoreCaptions				();

	u32			dwLastTimeRemains;
	string1024	warmup_message;
	string64	timelimit_message;
	void	ConvertTime2String				(string64 & str, u32 time);
public:
			CTAGameClCaptionsManager	();
			~CTAGameClCaptionsManager	();
	void	Init						(game_cl_CaptureTheArtefact* parent,
										CUIGameCTA* game_ui);
	void	ShowCaptions				();
	void	ResetCaptions				();
	void	CanCallBuySpawn				(bool can_call);
	void	CanSpawn					(bool can_spawn);
	void	CanCallBuy					(bool can_call);
	void	SetWinnerTeam				(ETeam wteam);

	u32		SetWarmupTime				(u32 current_warmup_time, u32 current_time);
	void	SetTimeLimit				(u32 time_limit, u32 current_time);

}; //end class CTAGameClCaptionsManager

#endif //GAME_CL_CTA_CAPTIONS_MANAGER