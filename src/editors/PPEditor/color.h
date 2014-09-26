#ifndef ColorH
#define ColorH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Mask.hpp>
#include <ExtCtrls.hpp>
#include "main.h"
#include "PostprocessAnimator.h"
#include <Dialogs.hpp>
#include "multi_edit.hpp"
#include "MXCtrls.hpp"
#include "base.h"

//---------------------------------------------------------------------------
class TAddColorForm : public TForm, public TPPPropEditor
{
__published:	// IDE-managed Components
    TGroupBox *GroupBox1;
    TMultiObjSpinEdit *RedValue;
    TMxLabel *RxLabel2;
    TPanel *Color;
    TColorDialog *ColorDialog;
	TMxLabel *RxLabel6;
	TMultiObjSpinEdit *GreenValue;
	TMxLabel *RxLabel10;
	TMultiObjSpinEdit *BlueValue;
	TMxLabel *labelIntensity;
	TMultiObjSpinEdit *IntensityValue;
	TMxLabel *MxLabel1;
	TMultiObjSpinEdit *TimeValue;
    void __fastcall CnahgeParam(TObject *Sender);
	void __fastcall ColorClick(TObject *Sender);
	void __fastcall TimeValueExit(TObject *Sender);
	void __fastcall TimeValueKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
private:	// User declarations
    void    		UpdateColor     ();
    bool			m_bLocked;
public:		// User declarations
	virtual void	Lock				(bool b){m_bLocked=b;}
	_pp_params		m_pp_params;
    
    __fastcall 		TAddColorForm(TComponent* Owner, _pp_params p);
    
    virtual void	ShowCurrent			(u32 keyIdx);
    virtual _pp_params GetTimeChannel	() {return m_pp_params;};
    virtual bool	DrawChannel			(_pp_params p);;
    virtual void    Clear           	();
    virtual TForm*	GetForm				() {return this;};
    virtual void	AddNew				(u32 keyIdx);
    virtual void	Remove				(u32 keyIdx);
    virtual void	RemoveAllKeys		();
    virtual void	CreateKey			(float t);
};
#endif
