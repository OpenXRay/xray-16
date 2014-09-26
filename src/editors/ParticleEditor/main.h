//---------------------------------------------------------------------------

#ifndef mainH
#define mainH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>         
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <Menus.hpp>
#include "ExtBtn.hpp"
#include "MxMenus.hpp"
#include "mxPlacemnt.hpp"
#include "RenderWindow.hpp"
//---------------------------------------------------------------------------
// refs

class ECORE_API TfrmMain : public TForm
{
__published:	// IDE-managed Components
        TPanel *paLeftBar;
        TPanel *paBottomBar;
    TPanel *paTools;
    TTimer *tmRefresh;
	TFormStorage *fsStorage;
    TPanel *paMain;
    TPanel *paTopBar;
	TPanel *paRender;
	TLabel *APHeadLabel2;
	TExtBtn *sbToolsMin;
	TD3DWindow *D3DWindow;
	TExtBtn *ebAllMin;
	TExtBtn *ebAllMax;
        void __fastcall FormCreate(TObject *Sender);
    void __fastcall D3DWindowResize(TObject *Sender);
    void __fastcall D3DWindowKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall D3DWindowKeyUp(TObject *Sender, WORD &Key,
          TShiftState Shift);
    void __fastcall sbToolsMinClick(TObject *Sender);
    void __fastcall TopClick(TObject *Sender);
    void __fastcall FormShow(TObject *Sender);
    void __fastcall tmRefreshTimer(TObject *Sender);
    void __fastcall D3DWindowPaint(TObject *Sender);
    void __fastcall D3DWindowKeyPress(TObject *Sender, char &Key);
    void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall D3DWindowChangeFocus(TObject *Sender);
	void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall D3DWindowMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall D3DWindowMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall D3DWindowMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
	void __fastcall ebAllMinClick(TObject *Sender);
	void __fastcall ebAllMaxClick(TObject *Sender);
	void __fastcall FormResize(TObject *Sender);
	void __fastcall paRenderResize(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
private:	// User declarations
    void __fastcall IdleHandler(TObject *Sender, bool &Done);

	TShiftState	ShiftKey;
    HINSTANCE 	m_HInstance;
public:		// User declarations
    __fastcall 		TfrmMain(TComponent* Owner);
    void __fastcall UpdateCaption();
    __inline void 	SetHInst(HINSTANCE inst){ m_HInstance=inst; }
    bool            IsFocused(){return D3DWindow->Focused();}
};
//---------------------------------------------------------------------------
extern /*ECORE_API*/ PACKAGE TfrmMain *frmMain;
//---------------------------------------------------------------------------
#endif
