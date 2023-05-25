//---------------------------------------------------------------------------
#ifndef LogFormH
#define LogFormH
//---------------------------------------------------------------------------

class UILogForm
{
public:
	static void AddMessage(TMsgDlgType mt, const xr_string &msg);
	static void AddMessage(const xr_string &msg) { AddMessage(mtCustom, msg); }
	static void AddDlgMessage(TMsgDlgType mt, const xr_string &msg);
	static void AddDlgMessage(const xr_string &msg) { AddDlgMessage(mtCustom, msg); }
	static void Show();
	static void Hide();
	static void Update();
	static void Destroy();

private:
	static xr_vector<xr_string> *List;
	static xr_vector<xr_string> *GetList();
	static bool bAutoScroll;
	static bool bOnlyError;
};
//---------------------------------------------------------------------------
#endif
