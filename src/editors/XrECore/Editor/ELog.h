//----------------------------------------------------
// file: Log.h
//----------------------------------------------------

#ifndef ELogH
#define ELogH
enum TMsgDlgType
{
	mtCustom = 0,
	mtError = 1,
	mtInformation = 2,
	mtConfirmation = 4,

};
enum TMsgDlgButtons
{
	mbYes = 1,
	mbNo = 2,
	mbCancel = 4,
	mbOK = 8,
	mrNone = 0,
	mrYes,
	mrNo,
	mrCancel,
	mrOK,
};
class ECORE_API CLog
{
public:
	bool in_use;

public:
	CLog() { in_use = false; }
	void Msg(TMsgDlgType mt, LPCSTR _Format, ...);
	int DlgMsg(TMsgDlgType mt, LPCSTR _Format, ...);
	int DlgMsg(TMsgDlgType mt, int btn, LPCSTR _Format, ...);
	void Close();
};

void ECORE_API ELogCallback(LPCSTR txt);

extern ECORE_API CLog ELog;

#endif /*_INCDEF_NETDEVICELOG_H_*/
