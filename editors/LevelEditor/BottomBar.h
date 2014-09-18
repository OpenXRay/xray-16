//---------------------------------------------------------------------------


#ifndef BottomBarH
#define BottomBarH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ExtBtn.hpp"
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include "CGAUGES.h"
#include <ExtCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class ECORE_API TfraBottomBar : public TFrame
{
__published:	// IDE-managed Components
	TFormStorage *fsStorage;
	TMxPopupMenu *pmOptions;
    TMenuItem *Quality1;
	TMenuItem *N25;
	TMenuItem *N50;
	TMenuItem *N75;
	TMenuItem *N100;
	TMenuItem *N125;
	TMenuItem *N150;
	TMenuItem *N200;
    TMenuItem *N2;
    TMenuItem *miDrawGrid;
	TMenuItem *Render1;
	TMenuItem *FillMode1;
	TMenuItem *miRenderFillSolid;
	TMenuItem *miRenderFillWireframe;
	TMenuItem *miRenderFillPoint;
	TMenuItem *ShadeMode1;
	TMenuItem *miRenderShadeGouraud;
	TMenuItem *miRenderShadeFlat;
	TMenuItem *miRenderEdgedFaces;
	TMenuItem *miRenderHWTransform;
	TMenuItem *miRenderLinearFilter;
	TMenuItem *miRenderWithTextures;
	TMenuItem *N7;
	TMenuItem *N8;
	TMenuItem *miLightScene;
	TMenuItem *miFog;
	TMenuItem *miRealTime;
    TMenuItem *N1;
	TPanel *paBottomBar;
	TPanel *paInfo;
	TPanel *paStatusBar;
	TPanel *paTools;
	TExtBtn *ebOptions;
	TExtBtn *ebLog;
	TExtBtn *ebStat;
	TExtBtn *ebStop;
	TPanel *paStatusLabel;
	TPanel *paStatus;
	TCGauge *cgProgress;
	TPanel *paSel;
	TPanel *paGridSquareSize;
	TPanel *paUICursor;
	TPanel *paCamera;
	TMenuItem *miDrawSafeRect;
	TMenuItem *miMuteSounds;
	TMenuItem *N5;
	TMenuItem *miWeather;
	TMenuItem *miWeatherNone;
	TMenuItem *N6;
	TExtBtn *ebMacro;
	TMxPopupMenu *pmMacro;
	TMenuItem *Device1;
	TMenuItem *TimeFactor1;
	TMenuItem *N0051;
	TMenuItem *N011;
	TMenuItem *N031;
	TMenuItem *N051;
	TMenuItem *N101;
	TMenuItem *N301;
	TMenuItem *N501;
	TMenuItem *N021;
	TMenuItem *N1001;
    void __fastcall ClickOptionsMenuItem(TObject *Sender);
    void __fastcall QualityClick(TObject *Sender);
    void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall ebLogClick(TObject *Sender);
	void __fastcall ebStopClick(TObject *Sender);
	void __fastcall ebStatClick(TObject *Sender);
	void __fastcall ebOptionsMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall pmOptionsPopup(TObject *Sender);
	void __fastcall miWeatherClick(TObject *Sender);
	void __fastcall ebMacroMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall N501Click(TObject *Sender);
private:	// User declarations
	void __fastcall MacroAssignClick(TObject *Sender);
	void __fastcall MacroClearClick(TObject *Sender);
	void __fastcall MacroExecuteClick(TObject *Sender);
	void __fastcall MacroLogCommandsClick(TObject *Sender);
	void __fastcall MacroEditCommandListClick(TObject *Sender);
public:		// User declarations
    __fastcall TfraBottomBar(TComponent* Owner);
    void RefreshBar(){;}
    void RedrawBar();
};
//---------------------------------------------------------------------------
extern PACKAGE TfraBottomBar *fraBottomBar;
//---------------------------------------------------------------------------
#endif
