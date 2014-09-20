//---------------------------------------------------------------------------


#ifndef TopBarH
#define TopBarH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>
#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
//---------------------------------------------------------------------------
class ECORE_API TfraTopBar : public TFrame
{
__published:	// IDE-managed Components
    TPanel *paTBEdit;
    TExtBtn *ebEditUndo;
    TExtBtn *ebEditRedo;
    TPanel *paTBAction;
    TExtBtn *ebActionMove;
    TExtBtn *ebActionRotate;
    TExtBtn *ebActionScale;
    TExtBtn *ebActionSelect;
    TExtBtn *ebActionAdd;
    TPanel *paSnap;
    TExtBtn *ebMSnap;
    TExtBtn *ebASnap;
	TExtBtn *ebCSParent;
    TPanel *paAxis;
    TExtBtn *ebAxisX;
    TExtBtn *ebAxisY;
    TExtBtn *ebAxisZ;
    TExtBtn *ebAxisZX;
    TExtBtn *ebGSnap;
    TExtBtn *ebOSnap;
    TPanel *paView;
    TExtBtn *ebViewFront;
    TExtBtn *ebViewLeft;
    TExtBtn *ebViewTop;
    TExtBtn *ebViewBack;
    TExtBtn *ebViewRight;
    TExtBtn *ebViewBottom;
	TExtBtn *ebNUScale;
	TPanel *Panel1;
	TExtBtn *ebZoomExtents;
	TExtBtn *ebZoomExtentsSelected;
	TExtBtn *ebVSnap;
	TFormStorage *fsStorage;
	TExtBtn *ebCameraPlane;
	TExtBtn *ebCameraArcBall;
	TExtBtn *ebCameraFly;
	TExtBtn *ebViewReset;
	TExtBtn *ebMTSnap;
	TExtBtn *ebNormalAlign;
    void __fastcall ebEditUndoClick(TObject *Sender);
    void __fastcall ebEditRedoClick(TObject *Sender);
    void __fastcall ActionClick(TObject *Sender);
    void __fastcall ebViewClick(TObject *Sender);
	void __fastcall ebZoomExtentsClick(TObject *Sender);
	void __fastcall ebZoomExtentsSelectedClick(TObject *Sender);
	void __fastcall ebCameraStyleClick(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall ebAxisClick(TObject *Sender);
	void __fastcall ebSettingsClick(TObject *Sender);
private:	// User declarations
public:		// User declarations
        __fastcall TfraTopBar(TComponent* Owner);
    void OnTimer();
    void __fastcall RefreshBar();
};
//---------------------------------------------------------------------------
extern PACKAGE TfraTopBar *fraTopBar;
//---------------------------------------------------------------------------
#endif
