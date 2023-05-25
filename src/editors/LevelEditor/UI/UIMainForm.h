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
	IC UIRenderForm *GetRenderForm() { return m_Render; }
	IC UILPropertiesFrom *GetPropertiesFrom() { return m_Properties; }

private:
	UITopBarForm *m_TopBar;
	UIRenderForm *m_Render;
	UIMainMenuForm *m_MainMenu;
	UILeftBarForm *m_LeftBar;
	UILPropertiesFrom *m_Properties;

private:
	void DrawContextMenu();
};
extern UIMainForm *MainForm;
