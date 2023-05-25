#pragma once
class ESceneGroupTool;
class UIGroupTool : public UIToolCustom
{
public:
	UIGroupTool();
	virtual ~UIGroupTool();
	virtual void Draw();
	virtual void OnDrawUI();
	ESceneGroupTool *ParentTools;

private:
	void MultiSelByRefObject(bool clear_prev);
	void SelByRefObject(bool flag);
	float m_selPercent;
	xr_string m_Current;
	bool m_ChooseGroup;
};