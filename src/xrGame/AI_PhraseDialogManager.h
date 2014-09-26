///////////////////////////////////////////////////////////////
// AI_PhraseDialogManager.h
// Класс, от которого наследуются NPC персонажи, ведущие диалог
// с актером
//
///////////////////////////////////////////////////////////////

#pragma once

#include "PhraseDialogManager.h"


class CAI_PhraseDialogManager: public CPhraseDialogManager
{
private:
	typedef CPhraseDialogManager inherited;
public:
					CAI_PhraseDialogManager			();
	virtual			~CAI_PhraseDialogManager		();

	virtual void	ReceivePhrase					(DIALOG_SHARED_PTR& phrase_dialog);
	virtual void	UpdateAvailableDialogs			(CPhraseDialogManager* partner);
	virtual void	AnswerPhrase					(DIALOG_SHARED_PTR& phrase_dialog);


	virtual void				SetStartDialog				(shared_str phrase_dialog);
	virtual void				SetDefaultStartDialog		(shared_str phrase_dialog);
	virtual shared_str	GetStartDialog						()								{return m_sStartDialog;}
	virtual void				RestoreDefaultStartDialog	();
protected:
	//диалог, если не NULL, то его персонаж запустит
	//при встрече с актером
	shared_str m_sStartDialog;
	shared_str m_sDefaultStartDialog;

	DEFINE_VECTOR(DIALOG_SHARED_PTR, DIALOG_SHARED_VECTOR, DIALOG_SHARED_IT);
	//список диалогов, на которые нужно ответить
	DIALOG_SHARED_VECTOR m_PendingDialogs;
};