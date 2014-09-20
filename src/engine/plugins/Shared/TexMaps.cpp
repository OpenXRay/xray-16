//-----------------------------------------------------------------------------
//  Texmaps
//-----------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include	"texmaps.h"
#include	"XRayMtlRes.h"

extern TCHAR *GetString(int id);

#define MAX_TEXTURE_CHANNELS	32

#define TEXMAPS_CLASS_ID 0x001200

static Class_ID TexmapsClassID(TEXMAPS_CLASS_ID, 0);

class OldTexmapsClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading) { 	return xr_new<Texmaps>((MtlBase*)NULL); }
	const TCHAR *	ClassName() { return GetString(IDS_DS_CLASSTEXMAPS); }
	SClass_ID		SuperClassID() { return REF_MAKER_CLASS_ID; }
	Class_ID 		ClassID() { return TexmapsClassID; }
	const TCHAR* 	Category() { return _T("");  }
	};


class TexmapsClassDesc:public ClassDesc {
	public:
	int 			IsPublic() { return 0; }
	void *			Create(BOOL loading) { 	return xr_new<Texmaps>((MtlBase*)NULL); }
	const TCHAR *	ClassName() { return GetString(IDS_DS_CLASSTEXMAPS); }
	SClass_ID		SuperClassID() { return TEXMAP_CONTAINER_CLASS_ID; }
	Class_ID 		ClassID() { return TexmapsClassID; }
	const TCHAR* 	Category() { return _T("");  }
	};

TexmapSlot::TexmapSlot() { 
	amount = 1.0f; 
	map = NULL; 
	mapOn = FALSE; 
	amtCtrl=NULL; 
	}

void TexmapSlot::Update(TimeValue t,Interval& ivalid) {
	if (IsActive()) 
		map->Update(t,ivalid);			
	if (amtCtrl) {
		amtCtrl->GetValue(t,&amount,ivalid);	
		}
	}

float TexmapSlot::GetAmount(TimeValue t) {
	Interval v;
	float f;
	if (amtCtrl) {
		amtCtrl->GetValue(t,&f,v);	
		return f;
		}
	else return amount;
	} 

Texmaps::Texmaps() {
	loadingOld = FALSE;
	client = NULL;
	}

					
Texmaps::Texmaps(MtlBase *mb) {
	loadingOld = FALSE;
	client = mb;
	}

SvGraphNodeReference Texmaps::SvTraverseAnimGraph(IGraphObjectManager *gom, Animatable *owner, int id, DWORD flags)
	{
	int i, nUsedSlots;

	if (!gom->TestFilter(SV_FILTER_MAPS))
		return SvGraphNodeReference();

	nUsedSlots = 0;
	for (i = 0; i < STD2_NMAX_TEXMAPS; i++)
		if (txmap[i].map)
			nUsedSlots++;

	if (nUsedSlots)
		return SvStdTraverseAnimGraph(gom, owner, id, flags);
	else
		return SvGraphNodeReference();
	}

static TexmapsClassDesc texmapsCD;
ClassDesc* GetTexmapsDesc() { return &texmapsCD;  }

static OldTexmapsClassDesc oldtexmapsCD;
ClassDesc* GetOldTexmapsDesc() { return &oldtexmapsCD;  }

Class_ID Texmaps::ClassID() { return TexmapsClassID; }

int Texmaps::NumSubs() { return STD2_NMAX_TEXMAPS*2; }  

int Texmaps::NumRefs() { return STD2_NMAX_TEXMAPS*2; }

Animatable* Texmaps::SubAnim(int i) {
	if (i&1)
		return txmap[i/2].map;
	else 
		return txmap[i/2].amtCtrl;
	}

TSTR Texmaps::SubAnimName(int i) {
	if (i&1)
		return client->GetSubTexmapTVName(i/2);
	else  {
		TSTR nm;
//		nm = GetString(texNameID[i/2]);
//		nm = textureChannelNames[ i/2 ];
		nm = txmap[ i/2 ].name;
		nm += TSTR(" ");
		nm += TSTR(GetString(IDS_DS_AMOUNT));
		return nm;
		}
	}

RefTargetHandle Texmaps::GetReference(int i) {
	if (i&1)
		return txmap[i/2].map;
	else 
		return txmap[i/2].amtCtrl;
	}

void Texmaps::SetReference(int i, RefTargetHandle rtarg) {
	if (loadingOld)
		txmap[i].map = (Texmap*)rtarg;
	else {
		if (i&1)
			txmap[i/2].map = (Texmap*)rtarg;
		else 
			txmap[i/2].amtCtrl = (Control*)rtarg;
		}
	}

void Texmaps::DeleteThis() { xr_delete((Texmaps*)this);}

RefResult Texmaps::NotifyRefChanged(Interval changeInt, RefTargetHandle hTarget, 
   PartID& partID, RefMessage message ) {
	switch (message) {
		case REFMSG_GET_PARAM_DIM: {
			GetParamDim *gpd = (GetParamDim*)partID;
			gpd->dim = defaultDim; 
			break;
			}
		case REFMSG_GET_PARAM_NAME: {
			GetParamName *gpn = (GetParamName*)partID;
			return REF_STOP; 
			}
		}
	return(REF_SUCCEED);
	}

void Texmaps::RescaleWorldUnits(float f) {
	if (TestAFlag(A_WORK1))
		return;
	SetAFlag(A_WORK1);
	// This code will be replaced in particular implementations
	for (int i=0; i<NumRefs(); i++) {
		if ( (i&1) ==0) 
			continue;  // skip the amount controllers
		ReferenceMaker *srm = GetReference(i);
		if (srm) {
			srm->RescaleWorldUnits(f);
			}
		}
		
	}

RefTargetHandle Texmaps::Clone(RemapDir &remap) {
	Texmaps *tm = xr_new<Texmaps>((MtlBase*)NULL);
	for (int i = 0; i<STD2_NMAX_TEXMAPS; i++) {
		tm->txmap[i].amount = txmap[i].amount;
		tm->txmap[i].mapOn = txmap[i].mapOn;
		tm->txmap[i].map = NULL;
		if (txmap[i].amtCtrl) 
			tm->ReplaceReference(2*i,remap.CloneRef(txmap[i].amtCtrl));
		if (txmap[i].map) 
			tm->ReplaceReference(2*i+1,remap.CloneRef(txmap[i].map));
		}
	BaseClone(this, tm, remap);
	return tm;
	}

#define TEX_OLD_ONOFF_CHUNK 0x5002
#define TEX_ONOFF_CHUNK 0x5003
#define TEX_AMT0 0x5100
#define TEX_AMT1 0x5101
#define TEX_AMT2 0x5102
#define TEX_AMT3 0x5103
#define TEX_AMT4 0x5104
#define TEX_AMT5 0x5105
#define TEX_AMT6 0x5106
#define TEX_AMT7 0x5107
#define TEX_AMT8 0x5108
#define TEX_AMT9 0x5109
#define TEX_AMTA 0x510A
#define TEX_AMTB 0x510B
#define TEX_AMTC 0x510C
#define TEX_AMTD 0x510D
#define TEX_AMTE 0x510E
#define TEX_AMTF 0x510F

IOResult Texmaps::Save(ISave *isave) { 
	isave->BeginChunk(TEX_ONOFF_CHUNK);
	ULONG nb,f=0;
	for ( int i=0; i<STD2_NMAX_TEXMAPS; i++) 
		if (txmap[i].mapOn) f|= (1<<i);
	isave->Write(&f,sizeof(f),&nb);			
	isave->EndChunk();

	for ( i=0; i<STD2_NMAX_TEXMAPS; i++) {
		if (txmap[i].amount!=1.0f) {
			isave->BeginChunk(TEX_AMT0+i);
			isave->Write(&txmap[i].amount,sizeof(float),&nb);			
			isave->EndChunk();
			}
		}
	return IO_OK;
	}

class TexmapsPostLoad : public PostLoadCallback {
	public:
		Texmaps *tm;
		TexmapsPostLoad(Texmaps *b) {tm=b;}
		void proc(ILoad *iload) {  tm->loadingOld = FALSE; xr_delete((TexmapsPostLoad*)this); } 
	};

	
IOResult Texmaps::Load(ILoad *iload) { 
	ULONG nb;
	int id;
	IOResult res;
	while (IO_OK==(res=iload->OpenChunk())) {
		switch(id = iload->CurChunkID())  {
			case TEX_OLD_ONOFF_CHUNK:
				iload->SetObsolete();
				iload->RegisterPostLoadCallback(xr_new<TexmapsPostLoad>(this));
				loadingOld = TRUE;
			case TEX_ONOFF_CHUNK:
				{
				ULONG f;
				res = iload->Read(&f,sizeof(f), &nb);
				for (int i=0; i<STD2_NMAX_TEXMAPS; i++) 
				    txmap[i].mapOn = (f&(1<<i))?1:0;
				}
				break;
			case TEX_AMT0: case TEX_AMT1:
			case TEX_AMT2:	case TEX_AMT3:
			case TEX_AMT4:	case TEX_AMT5:
			case TEX_AMT6:	case TEX_AMT7:
			case TEX_AMT8:	case TEX_AMT9:
			case TEX_AMTA:	case TEX_AMTB:
			case TEX_AMTC:	case TEX_AMTD:
			case TEX_AMTE:	case TEX_AMTF:
				res = iload->Read(&txmap[id-TEX_AMT0].amount,sizeof(float), &nb);
				break;
			}
		iload->CloseChunk();
		if (res!=IO_OK) 
			return res;
		}
	return IO_OK;
                                                      
	}
