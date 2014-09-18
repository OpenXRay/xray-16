//---------------------------------------------------------------------------

#include "stdafx.h"
#pragma hdrstop

#include "FrameEmitter.h"
#include "ShaderFunction.h"
#include "ParticleSystem.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "ElPgCtl"
#pragma link "ElXPThemedControl"
#pragma resource "*.dfm"
TfraEmitter *fraEmitter;
//---------------------------------------------------------------------------
__fastcall TfraEmitter::TfraEmitter(TComponent* Owner)
	: TFrame(Owner)
{
}
//---------------------------------------------------------------------------

void TfraEmitter::GetInfoFirst(const PS::SEmitterDef& E){
    pcEmitterType->ActivePageIndex	= E.m_EmitterType;
	// cone
	seConeDirH->ObjFirstInit		(rad2deg(E.m_ConeHPB.x));
    seConeDirP->ObjFirstInit		(rad2deg(E.m_ConeHPB.y));
    seConeDirB->ObjFirstInit		(rad2deg(E.m_ConeHPB.z));
    seConeAngle->ObjFirstInit		(rad2deg(E.m_ConeAngle));
    // sphere
    seSphereRadius->ObjFirstInit	(E.m_SphereRadius);
    // box
    seBoxSizeX->ObjFirstInit		(E.m_BoxSize.x);
    seBoxSizeY->ObjFirstInit		(E.m_BoxSize.y);
    seBoxSizeZ->ObjFirstInit		(E.m_BoxSize.z);
    // birth
    seBirthRate->ObjFirstInit		(E.m_fBirthRate);
    ebBirthFunc->Down				= E.m_Flags.is(PS_EM_BIRTHFUNC);
    seParticleLimit->ObjFirstInit 	(E.m_ParticleLimit);
	// burst
	cbBurst->ObjFirstInit			((E.m_Flags.is(PS_EM_BURST))?cbChecked:cbUnchecked);
    // play once
    cbPlayOnce->ObjFirstInit		((E.m_Flags.is(PS_EM_PLAY_ONCE))?cbChecked:cbUnchecked);
}
//---------------------------------------------------------------------------

void TfraEmitter::GetInfoNext(const PS::SEmitterDef& E){
    pcEmitterType->ActivePageIndex	= E.m_EmitterType;
	// cone
	seConeDirH->ObjNextInit			(rad2deg(E.m_ConeHPB.x));
    seConeDirP->ObjNextInit			(rad2deg(E.m_ConeHPB.y));
    seConeDirB->ObjNextInit			(rad2deg(E.m_ConeHPB.z));
    seConeAngle->ObjNextInit		(rad2deg(E.m_ConeAngle));
    // sphere
    seSphereRadius->ObjNextInit		(E.m_SphereRadius);
    // box
    seBoxSizeX->ObjNextInit			(E.m_BoxSize.x);
    seBoxSizeY->ObjNextInit			(E.m_BoxSize.y);
    seBoxSizeZ->ObjNextInit			(E.m_BoxSize.z);
    // birth
    seBirthRate->ObjNextInit		(E.m_fBirthRate);
    ebBirthFunc->Down				= E.m_Flags.is(PS_EM_BIRTHFUNC);
    seParticleLimit->ObjNextInit	(E.m_ParticleLimit);
	// burst
	cbBurst->ObjNextInit			((E.m_Flags.is(PS_EM_BURST))?cbChecked:cbUnchecked);
    // play once
    cbPlayOnce->ObjNextInit			((E.m_Flags.is(PS_EM_PLAY_ONCE))?cbChecked:cbUnchecked);
}
//---------------------------------------------------------------------------

void TfraEmitter::SetInfo(PS::SEmitterDef& E){
    E.m_EmitterType					= PS::SEmitter::EEmitterType(pcEmitterType->ActivePageIndex);
	// cone
    E.m_Flags.zero();
    E.m_Flags.set					(PS_EM_BIRTHFUNC,ebBirthFunc->Down);
    E.m_ConeHPB.set					(float(deg2rad(seConeDirH->Value)),
                                     float(deg2rad(seConeDirP->Value)),
                                     float(deg2rad(seConeDirB->Value)));
    E.UpdateConeOrientation			();
    E.m_ConeAngle 					= deg2rad(seConeAngle->Value);
    // sphere
    seSphereRadius->ObjApplyFloat	(E.m_SphereRadius);
    // box
    seBoxSizeX->ObjApplyFloat		(E.m_BoxSize.x);
    seBoxSizeY->ObjApplyFloat		(E.m_BoxSize.y);
    seBoxSizeZ->ObjApplyFloat		(E.m_BoxSize.z);
    // birth
    seBirthRate->ObjApplyFloat		(E.m_fBirthRate);
    seParticleLimit->ObjApplyFloat	(E.m_ParticleLimit);
	// burst
    E.m_Flags.set					(PS_EM_BURST,cbBurst->Checked);
    // play once
    E.m_Flags.set					(PS_EM_PLAY_ONCE,cbPlayOnce->Checked);
}
//---------------------------------------------------------------------------

