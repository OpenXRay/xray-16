//---------------------------------------------------------------------------
#ifndef ESound_SourceH
#define ESound_SourceH
//---------------------------------------------------------------------------

#include "CustomObject.h"

class ESoundSource: public CCustomObject
{
	typedef CCustomObject inherited;
	// static sound
	ref_sound		m_Source;
    shared_str		m_WAVName;
    CSound_params	m_Params;
    Fvector2		m_PlayTime;
    Fvector2		m_RandomPause;
    Fvector2		m_ActiveTime;
    u32				m_NextTime;
    u32				m_StopTime;
    // edit events
    void __stdcall 	OnChangeWAV		(PropValue* prop);
    void __stdcall 	OnChangeSource	(PropValue* prop);
	void __stdcall  OnControlClick	(ButtonValue* sender, bool& bModif, bool& bSafe);
    void			ResetSource		();
    enum{
    	flLooped	= (1<<0),
    	flPlaying	= (1<<1),
        flSimulating= (1<<2)
    };
    Flags32			m_Flags;
    enum ECommand{
    	stNothing,
    	stPlay,
        stStop,
        stPause,
        stSimulate,
    };
    ECommand		m_Command;
public:
	enum ESoundType{
    	stStaticSource = 0,
    };
    ESoundType		m_Type;
public:
    virtual const Fvector& GetPosition	()		 const				{ return m_Params.position; 	}
    virtual void 	SetPosition		(const Fvector& pos)	{ m_Params.position.set(pos); if (m_Source._feedback()) m_Source.set_position(m_Params.position);	}
public:
					ESoundSource	(LPVOID data, LPCSTR name);
	void 			Construct		(LPVOID data);
					~ESoundSource	();
    virtual bool	CanAttach		() {return true;}
    
    LPCSTR			GetSourceWAV	(){return *m_WAVName;}
    void			SetSourceWAV	(LPCSTR fname);
    void			Play			(){m_Command=stPlay;}
    void			Stop			(){m_Command=stStop;}
    void			Simulate		(){m_Command=stSimulate;}

	virtual void 	Render			(int priority, bool strictB2F);
	virtual bool 	RayPick			(float& dist, const Fvector& S, const Fvector& D, SRayPickInfo* pinf = NULL);
    virtual bool 	FrustumPick		(const CFrustum& frustum);
    virtual	void	OnFrame			();
  	virtual bool 		LoadStream			(IReader&);
  	virtual bool 		LoadLTX				(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream			(IWriter&);
  	virtual void 		SaveLTX				(CInifile& ini, LPCSTR sect_name);
	virtual bool 	GetBox			(Fbox& box) const;
	virtual void 	Scale			(Fvector& amount){;}
	virtual void	FillProp		(LPCSTR pref, PropItemVec& values);
	virtual bool 	GetSummaryInfo	(SSceneSummary* inf);
    virtual bool	ExportGame		(SExportStreams* data);
};

#endif
