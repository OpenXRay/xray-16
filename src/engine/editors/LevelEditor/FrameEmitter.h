//---------------------------------------------------------------------------


#ifndef FrameEmitterH
#define FrameEmitterH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "multi_edit.hpp"
#include <ComCtrls.hpp>
#include "multi_check.hpp"
//---------------------------------------------------------------------------
#include "particlesystem.h"
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include "ElPgCtl.hpp"
#include "ElXPThemedControl.hpp"
#include <ExtCtrls.hpp>
#include "ESceneCustomMTools.h"

class TfraEmitter : public TFrame
{
__published:	// IDE-managed Components
	TLabel *RxLabel20;
	TLabel *RxLabel22;
	TExtBtn *ebBirthFunc;
	TMultiObjSpinEdit *seBirthRate;
	TMultiObjSpinEdit *seParticleLimit;
	TLabel *RxLabel1;
	TMultiObjCheck *cbBurst;
	TLabel *RxLabel6;
	TMultiObjCheck *cbPlayOnce;
	TElPageControl *pcEmitterType;
	TElTabSheet *tsPoint1;
	TPanel *Panel3;
	TElTabSheet *tsCone;
	TPanel *Panel1;
	TElTabSheet *tsSphere;
	TPanel *Panel5;
	TElTabSheet *tsBox;
	TPanel *Panel4;
	TLabel *RxLabel42;
	TLabel *RxLabel4;
	TLabel *RxLabel35;
	TLabel *RxLabel36;
	TLabel *RxLabel37;
	TMultiObjSpinEdit *seConeAngle;
	TMultiObjSpinEdit *seConeDirH;
	TMultiObjSpinEdit *seConeDirP;
	TMultiObjSpinEdit *seConeDirB;
	TLabel *RxLabel38;
	TMultiObjSpinEdit *seSphereRadius;
	TLabel *RxLabel39;
	TLabel *RxLabel40;
	TLabel *RxLabel41;
	TMultiObjSpinEdit *seBoxSizeX;
	TMultiObjSpinEdit *seBoxSizeY;
	TMultiObjSpinEdit *seBoxSizeZ;
private:	// User declarations
public:		// User declarations
	__fastcall TfraEmitter(TComponent* Owner);
    void GetInfoFirst(const PS::SEmitterDef& E);
    void GetInfoNext(const PS::SEmitterDef& E);
    void SetInfo(PS::SEmitterDef& E);
    virtual void 	OnEnter	(){;}
    virtual void 	OnExit	(){;}
};
//---------------------------------------------------------------------------
extern PACKAGE TfraEmitter *fraEmitter;
//---------------------------------------------------------------------------
#endif
