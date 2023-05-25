#pragma once
class UIWayTool : public UIToolCustom
{
public:
	UIWayTool();
	virtual ~UIWayTool();
	virtual void Draw();
	IC bool IsAutoLink() const { return m_AutoLink; }
	IC void SetWayMode(bool mode) { m_WayMode = mode; }

private:
	bool m_WayMode;
	bool m_AutoLink;
};