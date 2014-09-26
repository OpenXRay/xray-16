#ifndef single_paramH
#define single_paramH
//---------------------------------------------------------------------------
#include <Mask.hpp>
#include "color.h"
#include "multi_edit.hpp"
#include "MXCtrls.hpp"
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
//---------------------------------------------------------------------------
class TAddFloatForm : public TForm, public TPPPropEditor
{
__published:	// IDE-managed Components
    TGroupBox *GroupBox1;
	TMxLabel *Label1;
	TMultiObjSpinEdit *Value1;
	TMxLabel *Label2;
	TMultiObjSpinEdit *Value2;
	TMxLabel *MxLabel2;
	TMultiObjSpinEdit *Value3;
	TMxLabel *Label3;
	TMxLabel *MxLabel1;
	TMultiObjSpinEdit *TimeValue;
	TEdit *cmTextureName;
	void __fastcall ChangeParam(TObject *Sender);
	void __fastcall TimeValueExit(TObject *Sender);
	void __fastcall TimeValueKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
	void __fastcall cmTextureNameChange(TObject *Sender);
private:	// User declarations
    pp_params                m_Param;
	_pp_params		m_pp_params;
    bool			m_bLocked;
public:		// User declarations
	virtual void	Lock				(bool b){m_bLocked=b;}
    __fastcall 		TAddFloatForm		(TComponent* Owner, pp_params param);

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
