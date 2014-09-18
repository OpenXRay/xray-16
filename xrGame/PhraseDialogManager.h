#pragma once

#include "PhraseDialogDefs.h"

class CPhraseDialogManager
{
public:
									CPhraseDialogManager			(void);
	virtual							~CPhraseDialogManager			(void);

	virtual void					InitDialog			(CPhraseDialogManager* dialog_partner, DIALOG_SHARED_PTR& phrase_dialog);
	virtual void					AddDialog			(DIALOG_SHARED_PTR& phrase_dialog);

	//получение фразы, виртуальная функция, 
	//должна быть переопределена для сталкеров и актера
	virtual void					ReceivePhrase		(DIALOG_SHARED_PTR& phrase_dialog);
	//ответить на сказанную фразу в диалоге
	virtual void					SayPhrase			(DIALOG_SHARED_PTR& phrase_dialog, const shared_str& phrase_id);

	//виртуальная функция, заполняет массив, тем диалогами, которые
	//персонаж может инициировать в данный момент
	virtual void					UpdateAvailableDialogs(CPhraseDialogManager* partner);

	DEFINE_VECTOR					(DIALOG_SHARED_PTR, DIALOG_VECTOR, DIALOG_VECTOR_IT);
	const DIALOG_VECTOR&			AvailableDialogs	() {return m_AvailableDialogs;}
	const DIALOG_SHARED_PTR&		GetDialogByID		(const shared_str& dialog_id) const;
	bool							HaveAvailableDialog	(const shared_str& dialog_id) const;

protected:
	virtual bool					AddAvailableDialog	(shared_str dialog_id, CPhraseDialogManager* partner);
	
	//буфферный список диалогов, которые были проверены
	//во время UpdateAvailableDialogs
	DIALOG_ID_VECTOR				m_CheckedDialogs;

	//список активных диалогов
	DIALOG_VECTOR					m_ActiveDialogs;
	//список доступных диалогов
	DIALOG_VECTOR					m_AvailableDialogs;
};