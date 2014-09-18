#include "pch_script.h"
#include "artefact.h"
#include "MercuryBall.h"
#include "GraviArtifact.h"
#include "BlackDrops.h"
#include "Needles.h"
#include "BastArtifact.h"
#include "BlackGraviArtifact.h"
#include "DummyArtifact.h"
#include "ZudaArtifact.h"
#include "ThornArtifact.h"
#include "FadedBall.h"
#include "ElectricBall.h"
#include "RustyHairArtifact.h"
#include "GalantineArtifact.h"
#include "cta_game_artefact.h"

using namespace luabind;

#pragma optimize("s",on)
void CArtefact::script_register(lua_State *L)
{
	module(L)
	[
		class_<CArtefact			,CGameObject>("CArtefact")
		.def(						constructor<>() )
		.def("FollowByPath",		&CArtefact::FollowByPath)
		.def("SwitchVisibility",	&CArtefact::SwitchVisibility)
		.def("GetAfRank",			&CArtefact::GetAfRank),

		class_<CMercuryBall			,CArtefact>("CMercuryBall").def		(constructor<>()),
		class_<CBlackDrops			,CArtefact>("CBlackDrops").def		(constructor<>()),
		class_<CBlackGraviArtefact	,CArtefact>("CBlackGraviArtefact").def(constructor<>()),
		class_<CBastArtefact		,CArtefact>("CBastArtefact").def		(constructor<>()),
		class_<CDummyArtefact		,CArtefact>("CDummyArtefact").def		(constructor<>()),
		class_<CZudaArtefact		,CArtefact>("CZudaArtefact").def		(constructor<>()),
		class_<CThornArtefact		,CArtefact>("CThornArtefact").def		(constructor<>()),
		class_<CFadedBall			,CArtefact>("CFadedBall").def			(constructor<>()),
		class_<CElectricBall		,CArtefact>("CElectricBall").def		(constructor<>()),
		class_<CRustyHairArtefact	,CArtefact>("CRustyHairArtefact").def	(constructor<>()),
		class_<CGalantineArtefact	,CArtefact>("CGalantineArtefact").def	(constructor<>()),
		class_<CGraviArtefact		,CArtefact>("CGraviArtefact").def		(constructor<>())
	];
}
