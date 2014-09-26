//---------------------------------------------------------------------------
#ifndef ESound_EnvironmentH
#define ESound_EnvironmentH
//---------------------------------------------------------------------------

#include "CustomObject.h"
#include "EShape.h"
//#include "xrServer_Objects_Alife_All.h"

class ESoundEnvironment: public CEditShape
{
	typedef CCustomObject inherited;

	friend class		CLevelSoundManager;
	// Env
    shared_str			m_EnvInner;
	shared_str			m_EnvOuter;

    void __stdcall 		OnChangeEnvs	(PropValue* prop);
public:
    void				get_box			(Fmatrix& m);
public:
						ESoundEnvironment(LPVOID data, LPCSTR name);
	void 				Construct		(LPVOID data);
						~ESoundEnvironment();
    virtual bool		CanAttach		() {return true;}
    virtual void		OnUpdateTransform();
                        
  	virtual bool 		LoadStream			(IReader&);
  	virtual bool 		LoadLTX				(CInifile& ini, LPCSTR sect_name);
	virtual void 		SaveStream			(IWriter&);
  	virtual void 		SaveLTX				(CInifile& ini, LPCSTR sect_name);

	virtual void		FillProp		(LPCSTR pref, PropItemVec& values);
	virtual bool 		GetSummaryInfo	(SSceneSummary* inf);
	virtual void 		OnSceneUpdate	();
};

#endif
