//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "PERFGRAP.h"
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ToolWin.hpp>
#include <float_constructor.h>
#include <Dialogs.hpp>
#include <ImgList.hpp>
#include "MXCtrls.hpp"
#include <Menus.hpp>
#define XRCORE_API __declspec(dllimport)

#include "base.h"


class TPPPropEditor;

class TMainForm : public TForm
{
__published:
    TPanel *Panel;
    TToolBar *ToolBar1;
    TStatusBar *StatusBar;
    TImage *Image;
    TToolButton *NewEffectButton;
    TToolButton *ToolButton2;
    TToolButton *LoadButton;
    TToolButton *SaveButton;
    TTabControl *TabControl;
    TSaveDialog *SaveDialog;
    TOpenDialog *OpenDialog;
    TImageList *ImageList;
	TListBox *PointList;
	TMxLabel *RxLabel1;
	TPanel *WorkArea;
	TButton *btnAddKey;
	TButton *btnRemoveKey;
	TButton *ClearAll;
	TButton *copyFrom;
	TPopupMenu *PopupCopyFrom;
	TMenuItem *Addcolor1;
	TMenuItem *BaseColor1;
	TMenuItem *Graycolor1;
	TMenuItem *Duality1;
	TMenuItem *Noise1;
	TMenuItem *Blur1;
	TMainMenu *MainMenu1;
	TMenuItem *File1;
	TMenuItem *Save1;
	TMenuItem *Load1;
	TMenuItem *New1;
	TMenuItem *N1;
    void __fastcall FormResize(TObject *Sender);
    void __fastcall NewEffectButtonClick(TObject *Sender);
    void __fastcall FormDestroy(TObject *Sender);
    void __fastcall FormPaint(TObject *Sender);
    void __fastcall ImageMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall GrayColorClick(TObject *Sender);
    void __fastcall TabControlChange(TObject *Sender);
    void __fastcall LoadButtonClick(TObject *Sender);
    void __fastcall SaveButtonClick(TObject *Sender);
	void __fastcall PointListClick(TObject *Sender);
	void __fastcall btnAddKeyClick(TObject *Sender);
	void __fastcall btnRemoveKeyClick(TObject *Sender);
	void __fastcall ClearAllClick(TObject *Sender);
	void __fastcall copyFromClick(TObject *Sender);
	void __fastcall PopupCopyFromChange(TObject *Sender, TMenuItem *Source,
          bool Rebuild);
	void __fastcall PopupClick(TObject *Sender);
private:	// User declarations
	TPPPropEditor*					m_props[6];
    SPPInfo         				m_Params;
    TPPPropEditor*					m_ActiveShowForm;
    float           				m_Marker;
public:		// User declarations
    __fastcall 						TMainForm			(TComponent* Owner);
    void            				SetMarkerPosition   (float time);
    void            				UpdateGraph         ();
    void							FillPointList		();
    void							PointListSetTime	(int idx, float time);
    CPostprocessAnimator*			m_Animator;
};
//---------------------------------------------------------------------------
extern PACKAGE TMainForm *MainForm;
//---------------------------------------------------------------------------
#endif
