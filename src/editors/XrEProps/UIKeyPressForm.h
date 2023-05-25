#pragma once
class XREPROPS_API UIKeyPressForm : public XrUI
{
public:
	UIKeyPressForm();
	virtual ~UIKeyPressForm();
	virtual void Draw();

public:
	static void Update(float timeGlobal);
	static void Show();
	static bool SetResult(const xr_shortcut &result);
	static bool GetResult(bool &Ok, xr_shortcut &result);

private:
	xr_shortcut m_Resutl;
	float m_TimeGlobal;
	static UIKeyPressForm *Form;
	bool m_Ok;
};