//---------------------------------------------------------------------------
#ifndef FrameWayPointH
#define FrameWayPointH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <Buttons.hpp>
#include <ExtCtrls.hpp>

#include "ExtBtn.hpp"
#include "mxPlacemnt.hpp"
#include "ESceneCustomMTools.h"
// refs
class CEditObject;
//---------------------------------------------------------------------------
class TfraWayPoint : public TForm
{
__published:	// IDE-managed Components
    TPanel *paCommands;
	TLabel *APHeadLabel1;
	TExtBtn *ExtBtn2;
	TFormStorage *fsStorage;
	TExtBtn *ebModeWay;
	TPanel *paLink;
	TLabel *Label1;
	TExtBtn *ExtBtn3;
	TExtBtn *ebInvertLink;
	TExtBtn *ebAdd1Link;
	TExtBtn *ebRemoveLinks;
	TExtBtn *ebAdd2Link;
	TExtBtn *ebModePoint;
	TExtBtn *ebAutoLink;
	TBevel *Bevel1;
	TExtBtn *ebConvert1;
	TExtBtn *ebConvert2;
    void __fastcall PanelMinClick(TObject *Sender);
    void __fastcall ExpandClick(TObject *Sender);
	void __fastcall ebAdd1LinksClick(TObject *Sender);
	void __fastcall ebRemoveLinksClick(TObject *Sender);
	void __fastcall ebAdd2LinkClick(TObject *Sender);
	void __fastcall ebInvertLinkClick(TObject *Sender);
	void __fastcall ebModeWayClick(TObject *Sender);
	void __fastcall ebConvert1Click(TObject *Sender);
	void __fastcall ebConvert2Click(TObject *Sender);
private:	// User declarations
public:		// User declarations
	__fastcall TfraWayPoint(TComponent* Owner);
};
//---------------------------------------------------------------------------
#endif
