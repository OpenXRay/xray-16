#pragma once
class UIMainForm : public XrUI
{
public:
	UIMainForm();
	virtual ~UIMainForm();
	virtual void Draw();
	bool Frame();
	IC UILeftBarForm *GetLeftBarForm() { return m_LeftBar; }
	IC UITopBarForm *GetTopBarForm() { return m_TopBar; }

private:
	UITopBarForm *m_TopBar;
	UIRenderForm *m_Render;
	UIMainMenuForm *m_MainMenu;
	UILeftBarForm *m_LeftBar;
	UIRightBarForm *m_RightBar;
};
extern UIMainForm *MainForm;
