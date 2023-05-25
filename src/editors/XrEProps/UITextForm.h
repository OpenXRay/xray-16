#pragma once
class XREPROPS_API UITextForm : public XrUI
{
public:
	UITextForm(const char *str);
	virtual ~UITextForm();
	virtual void Draw();
	static void RunEditor(const char *str);
	static void Update();
	static bool GetResult(bool &change, xr_string &result);

private:
	void CLBOk();
	void CLBCancel();

	void CLBLoad();
	void CLBSave();
	void CLBClear();

private:
	bool m_Ok;
	xr_string m_Text;
	string4096 m_EditText;

	static UITextForm *Form;
};
