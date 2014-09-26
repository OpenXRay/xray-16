//---------------------------------------------------------------------------

#ifndef MinimapEditorH
#define MinimapEditorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "ExtBtn.hpp"
#include <ExtCtrls.hpp>
#include "MXCtrls.hpp"
#include "ElEdits.hpp"
#include "ElSpin.hpp"
#include "ElXPThemedControl.hpp"
#include "ElACtrls.hpp"

//---------------------------------------------------------------------------
class TTMinimapEditor : public TForm
{
__published:	// IDE-managed Components
    TPanel *Panel1;
    TExtBtn *btnLoad;
    TMxPanel *imgPanel;
    TMxLabel *MxLabel1;
    TElFloatSpinEdit *ElFloatSpinEditX1;
    TElFloatSpinEdit *ElFloatSpinEditZ1;
    TElFloatSpinEdit *ElFloatSpinEditX2;
    TElFloatSpinEdit *ElFloatSpinEditZ2;
    TMxLabel *MxLabel2;
    TMxLabel *MxLabel3;
    TElAdvancedEdit *result_edit;
    void __fastcall btnLoadClick(TObject *Sender);
    void __fastcall imgPanelPaint(TObject *Sender);
    void __fastcall imgPanelResize(TObject *Sender);
    void __fastcall imgPanelMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall ElFloatSpinEditX1Change(TObject *Sender);
private:	// User declarations
static TTMinimapEditor* form;
    U32Vec      image_data;
    u32         image_w;
    u32         image_h;
    u32         image_a;
    Fbox2       map_bb;
    Fbox2       map_bb_loaded;
    Ivector2    image_draw_size;
      void      ApplyPoints(bool to_controls);
public:		// User declarations
    __fastcall TTMinimapEditor(TComponent* Owner);

    static void __fastcall Show			    ();
    static void __fastcall Hide			    ();
    static bool __fastcall Visible			(){return !!form;}
};
//---------------------------------------------------------------------------
//. extern PACKAGE TTMinimapEditor *TMinimapEditor;
//---------------------------------------------------------------------------
#endif
