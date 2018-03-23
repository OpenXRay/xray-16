#include "pch.hpp"
#include "GameType.h"
#include "xrServerEntities/gametype_chooser.h"

namespace XRay::Editor::Controls
{
bool GameType::Run(pcstr title, GameTypeChooser* data)
{
    gameTypes = data;

    checkSingle->Checked = gameTypes->MatchType(eGameIDSingle);
    checkDM->Checked = gameTypes->MatchType(eGameIDDeathmatch);
    checkTDM->Checked = gameTypes->MatchType(eGameIDTeamDeathmatch);
    checkAfHunt->Checked = gameTypes->MatchType(eGameIDArtefactHunt);
    checkCTA->Checked = gameTypes->MatchType(eGameIDCaptureTheArtefact);

    return ShowDialog() == Windows::Forms::DialogResult::OK;
}

System::Void GameType::buttonOk_Click(System::Object^ sender, System::EventArgs^ e)
{
    gameTypes->m_GameType.zero();
    gameTypes->m_GameType.set(eGameIDSingle, checkSingle->Checked);
    gameTypes->m_GameType.set(eGameIDDeathmatch, checkDM->Checked);
    gameTypes->m_GameType.set(eGameIDTeamDeathmatch, checkTDM->Checked);
    gameTypes->m_GameType.set(eGameIDArtefactHunt, checkAfHunt->Checked);
    gameTypes->m_GameType.set(eGameIDCaptureTheArtefact, checkCTA->Checked);
}
} // namespace XRay::Editor::Controls
