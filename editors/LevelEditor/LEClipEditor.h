#ifndef LEClipEditorH
#define LEClipEditorH

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
#include "SkeletonAnimated.h"

// refs
class CEditableObject;
class CAnimationClip;

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
	TMxPanel *paFXs;
	TBevel *Bevel21;
	TExtBtn *ebSync;
	TExtBtn *ebClear;
	TPanel *paAnimSelect;
	TBevel *Bevel22;
	TBevel *Bevel23;
	TSplitter *Splitter1;
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

protected:
    enum{
    	flRT_RepaintClips		= (1<<0),
    	flRT_UpdateProperties	= (1<<1),
    	flRT_UpdateClips		= (1<<2),
    	flRT_Playing			= (1<<3),
    	flRT_PlayingLooped		= (1<<4),
    };
    Flags32					m_RTFlags;
	
    TMxLabel* 				m_LB[4];
    CKinematicsAnimated* 	m_RenderObject;
    
	DEFINE_VECTOR		(CAnimationClip*,AnimClipVec,AnimClipIt);
    AnimClipVec			clips;
    CAnimationClip*		sel_clip; 
    u32					play_clip;
    TProperties*		m_ClipProps;
    TItemList*			m_ClipList;

    void				PlayAnimation	(CAnimationClip* clip);
    
	void 				RemoveAllClips	();
	void 				LoadClips		();
	void 				SaveClips		();
	void 				InsertClip		();
	void 				AppendClip		();
	void				RemoveClip		(CAnimationClip* clip);
    void				SelectClip		(CAnimationClip* clip);

    CAnimationClip*		FindClip		(float t);
    CAnimationClip*		FindClip		(int x);
    
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

    void				ShowEditor		(CKinematicsAnimated* O);
    void				HideEditor		();

	virtual void		OnFrame			(void);
    TItemList*			m_ObjectItems;
};

struct AnimClipItem
{
    MotionID		mid;
    AnimClipItem	(){}
	void			clear			()			{mid.invalidate();}
	bool			valid			() const	{return mid.valid();}
};

class CAnimationClip
{
public:
    AnimClipItem	animItems[4];
public:
	shared_str		name;
    float			start_time;
    float 			length;
    s32				idx;
    TClipMaker* 	owner;
public:
                    CAnimationClip	(LPCSTR name, TClipMaker* owner);
                    CAnimationClip	(TClipMaker* owner);
                    ~CAnimationClip	();
    float			Length			() const {return length;}
    const float&	StartTime		() const {return start_time;}

    int				PWidthUI		(){return Length()*owner->m_Zoom;}
    int				PLeftUI			(){return StartTime()*owner->m_Zoom;}
    int				PRightUI		(){return PLeftUI()+PWidthUI();}

public:
	void			Save			(IWriter& F){R_ASSERT(0);}
	bool			Load			(IReader& F){R_ASSERT(0);return true;}
    
    void			SetCycle		(MotionID mid, u16 part_id, u8 part_count);
};
extern TClipMaker* g_clip_maker;
#endif
