#include "stdafx.h"
#pragma hdrstop

#include "GameTypeForm.h"
#include "../../xrServerEntities/gametype_chooser.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ExtBtn"
#pragma resource "*.dfm"
TfmGameType *fmGameType = NULL;
//---------------------------------------------------------------------------
bool __fastcall TfmGameType::Run(const char* title, GameTypeChooser* data)
{
    m_data = data;
    cbSingle->Checked 			= m_data->MatchType(eGameIDSingle);
    cbDeathMatch->Checked 		= m_data->MatchType(eGameIDDeathmatch);
    cbTeamDeathMatch->Checked 	= m_data->MatchType(eGameIDTeamDeathmatch);
    cbArtefactHunt->Checked 	= m_data->MatchType(eGameIDArtefactHunt);
    cbCTA->Checked 				= m_data->MatchType(eGameIDCaptureTheArtefact);

    return (ShowModal()==mrOk);
}

bool gameTypeRun(const char* title, GameTypeChooser* data)
{
	fmGameType = xr_new<TfmGameType>((TComponent*)0);
    bool res = fmGameType->Run(title, data);
    xr_delete(fmGameType);
    return res;
}

__fastcall TfmGameType::TfmGameType(TComponent* Owner)
	: TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TfmGameType::ebOkClick(TObject *Sender)
{
	m_data->m_GameType.zero	();
    m_data->m_GameType.set	(eGameIDSingle,cbSingle->Checked);
    m_data->m_GameType.set	(eGameIDDeathmatch,cbDeathMatch->Checked);
    m_data->m_GameType.set	(eGameIDTeamDeathmatch,cbTeamDeathMatch->Checked);
    m_data->m_GameType.set	(eGameIDArtefactHunt,cbArtefactHunt->Checked);
    m_data->m_GameType.set	(eGameIDCaptureTheArtefact,cbCTA->Checked);
    Close					();
    ModalResult 			= mrOk;
}
//---------------------------------------------------------------------------

void __fastcall TfmGameType::ebCancelClick(TObject *Sender)
{
    Close();
    ModalResult = mrCancel;
}
//---------------------------------------------------------------------------
