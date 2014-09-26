//----------------------------------------------------
// file: Log.h
//----------------------------------------------------

#ifndef ELogH
#define ELogH

class ECORE_API CLog{
public:
	bool 		in_use;
public:
				CLog	(){in_use=false;}
	void 		Msg   	(TMsgDlgType mt, LPCSTR _Format, ...);
	int 		DlgMsg 	(TMsgDlgType mt, LPCSTR _Format, ...);
	int 		DlgMsg 	(TMsgDlgType mt, TMsgDlgButtons btn, LPCSTR _Format, ...);
};

void ECORE_API ELogCallback(LPCSTR txt);

extern ECORE_API CLog ELog;

#endif /*_INCDEF_NETDEVICELOG_H_*/

