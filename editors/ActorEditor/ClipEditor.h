//---------------------------------------------------------------------------
#ifndef ClipEditorH
#define ClipEditorH
#include "mxPlacemnt.hpp"
#include <Classes.hpp>
#include "ExtBtn.hpp"
#include "MXCtrls.hpp"
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <Forms.hpp>
#include "Gradient.hpp"
#include "ElScrollBar.hpp"
#include "ElXPThemedControl.hpp"
#include "MxMenus.hpp"
#include <Menus.hpp>
#include <StdCtrls.hpp>
#include "multi_edit.hpp"
#include "../xrEProps/PropertiesList.h"
#include "motion.h"
#include "ElTrackBar.hpp"

// refs
class CEditableObject;
//---------------------------------------------------------------------------
class TClipMaker: public TForm,
	public pureFrame
{
__published:	// IDE-managed Components
	TFormStorage *fsStorage;
	TPanel *paB;
	TPanel *paBase;
	TPanel *paClipProps;
	TBevel *Bevel20;
	TPanel *paA;
	TPanel *Panel1;
	TMxLabel *MxLabel1;
	TMxLabel *MxLabel2;
	TMxLabel *MxLabel3;
	TBevel *Bevel2;
	TBevel *Bevel4;
	TBevel *Bevel5;
	TBevel *Bevel10;
	TBevel *Bevel11;
	TBevel *Bevel12;
	TMxLabel *MxLabel4;
	TMxLabel *lbBPName0;
	TBevel *Bevel13;
	TMxLabel *lbBPName1;
	TMxLabel *lbBPName2;
	TMxLabel *lbBPName3;
	TBevel *Bevel15;
	TBevel *Bevel16;
	TScrollBox *sbBase;
	TMxPanel *paFrame;
	TBevel *Bevel6;
	TBevel *Bevel7;
	TBevel *Bevel8;
	TBevel *Bevel1;
	TBevel *Bevel3;
	TMxPanel *paClips;
	TBevel *Bevel9;
	TMxPanel *gtClip;
	TMxPanel *paBP3;
	TMxPanel *paBP2;
	TMxPanel *paBP1;
	TMxPanel *paBP0;
	TBevel *Bevel18;
	TPanel *Panel2;
	TExtBtn *ebPrevClip;
	TExtBtn *ebPlay;
	TExtBtn *ebPlayCycle;
	TExtBtn *ebStop;
	TExtBtn *ebNextClip;
	TBevel *Bevel17;
	TBevel *Bevel19;
	TPanel *paClipList;
	TPanel *Panel3;
	TPanel *Panel4;
	TExtBtn *ebInsertClip;
	TExtBtn *ebAppendClip;
	TExtBtn *ebTrash;
	TExtBtn *ebLoadClips;
	TExtBtn *ebSaveClips;
	TBevel *Bevel14;
	TMxLabel *MxLabel5;
	TMxPanel *paFXs;
	TBevel *Bevel21;
	TExtBtn *ebSync;
	TExtBtn *ebClear;
	TElTrackBar *timeTrackBar;
	void __fastcall ebInsertClipClick(TObject *Sender);
	void __fastcall gtClipPaint(TObject *Sender);
	void __fastcall ebAppendClipClick(TObject *Sender);
	void __fastcall BPOnPaint(TObject *Sender);
	void __fastcall FormCreate(TObject *Sender);
	void __fastcall FormDestroy(TObject *Sender);
	void __fastcall fsStorageRestorePlacement(TObject *Sender);
	void __fastcall fsStorageSavePlacement(TObject *Sender);
	void __fastcall ClipDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall ClipDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall ClipMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ebPrevClipClick(TObject *Sender);
	void __fastcall ebNextClipClick(TObject *Sender);
	void __fastcall ebPlayClick(TObject *Sender);
	void __fastcall ebPlayCycleClick(TObject *Sender);
	void __fastcall ebStopClick(TObject *Sender);
	void __fastcall ClipMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
	void __fastcall ClipMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall FormShow(TObject *Sender);
	void __fastcall BPDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall BPDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall BPMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall BPMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
	void __fastcall ebTrashClick(TObject *Sender);
	void __fastcall ebTrashDragOver(TObject *Sender, TObject *Source, int X,
          int Y, TDragState State, bool &Accept);
	void __fastcall ClipPaint(TObject *Sender);
	void __fastcall ebTrashDragDrop(TObject *Sender, TObject *Source, int X,
          int Y);
	void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
	void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
	void __fastcall ebLoadClipsClick(TObject *Sender);
	void __fastcall ebSaveClipsClick(TObject *Sender);
	void __fastcall BPEndDrag(TObject *Sender, TObject *Target, int X,
          int Y);
	void __fastcall BPStartDrag(TObject *Sender, TDragObject *&DragObject);
	void __fastcall ClipStartDrag(TObject *Sender,
          TDragObject *&DragObject);
	void __fastcall ClipEndDrag(TObject *Sender, TObject *Target, int X,
          int Y);
	void __fastcall ebSyncClick(TObject *Sender);
	void __fastcall ebClearClick(TObject *Sender);
public:
    xr_string 			m_ClipFName;
	class CUIClip: public CClip{
    public:                               
    	float			run_time;
    	s32				idx;
        TClipMaker* 	owner;
    public:
        				CUIClip			(LPCSTR name, TClipMaker* owner, float r_t);
        				CUIClip			(TClipMaker* own, float r_t){owner=own;run_time=r_t;fx_power=1.f;}
        				~CUIClip		();
        int				PWidth			(){return length*owner->m_Zoom;}
        int				PLeft			(){return run_time*owner->m_Zoom;}
        int				PRight			(){return PLeft()+PWidth();}
        float			Length			(){return length;}
        float			RunTime			(){return run_time;}
        AnsiString		CycleName		(u16 bp){VERIFY(bp<4); 	return *cycles[bp].name;}
        u16				CycleSlot		(u16 bp){VERIFY(bp<4); 	return cycles[bp].slot;	}
        AnsiString		FXName			()      {				return *fx.name;		}
        u16				FXSlot			()      {				return fx.slot;			}
        void			SetCycle		(LPCSTR name, u16 part, u16 slot);
        void			SetFX			(LPCSTR name, u16 slot);
    };
protected:
    enum{
    	flRT_RepaintClips		= (1<<0),
    	flRT_UpdateProperties	= (1<<1),
    	flRT_UpdateClips		= (1<<2),
    	flRT_Playing			= (1<<3),
    	flRT_PlayingLooped		= (1<<4),
    };
    Flags32				m_RTFlags;
	
    TMxLabel* 			m_LB[4];
    CEditableObject* 	m_CurrentObject;
    
	DEFINE_VECTOR		(CUIClip*,UIClipVec,UIClipIt);
    UIClipVec			clips;
    CUIClip*			sel_clip; 
    u32					play_clip;
    TProperties*		m_ClipProps;
    TItemList*			m_ClipList;

    void				PlayAnimation	(CUIClip* clip);
    
	void 				RemoveAllClips	();
	void 				LoadClips		();
	void 				SaveClips		();
	void 				InsertClip		();
	void 				AppendClip		();
	void				RemoveClip		(CUIClip* clip);
    void				SelectClip		(CUIClip* clip);

    CUIClip*			FindClip		(float t);
    CUIClip*			FindClip		(int x);
    
    void				RealRepaintClips();
    void				RepaintClips	(bool bForced=false){m_RTFlags.set(flRT_RepaintClips,TRUE); if(bForced) RealRepaintClips(); }
	void 				RealUpdateProperties();
    void				UpdateProperties(bool bForced=false){m_RTFlags.set(flRT_UpdateProperties,TRUE); if(bForced) RealUpdateProperties(); }
	void 				RealUpdateClips	();
    void				UpdateClips		(bool bForced=false, bool bRepaint=true){m_RTFlags.set(flRT_UpdateClips,TRUE); m_RTFlags.set(flRT_RepaintClips,bRepaint); if(bForced) RealUpdateClips(); }

    void				Clear			();
               
	void __stdcall  	OnZoomChange		(PropValue* V);
    void __stdcall  	OnNameChange		(PropValue* V);
    void __stdcall  	OnClipLengthChange	(PropValue* V);

    void __stdcall  	OnClipItemFocused	(ListItemsVec& items);
public:
    float				m_CurrentPlayTime;
    float				m_TotalLength;
    float				m_Zoom;

    void				Play			(BOOL bLoop);
    void				Stop			();
public:		// User declarations
	__fastcall 			TClipMaker		(TComponent* Owner);

    static TClipMaker*	CreateForm		();
    static void			DestroyForm		(TClipMaker* form);

    void				ShowEditor		(CEditableObject* O);
    void				HideEditor		();

	virtual void		OnFrame			(void);
};
//---------------------------------------------------------------------------
#endif
