#pragma once
class UISpawnTool : public UIToolCustom
{
public:
	UISpawnTool();
	virtual ~UISpawnTool();
	virtual void Draw();
	void SelByRefObject(bool flag);
	void MultiSelByRefObject(bool clear_prev);
	IC const char *Current() { return m_Current; }
	IC void SetAttachObject(bool AttachObject) { m_AttachObject = AttachObject; }
	IC bool IsAttachObject() const { return m_AttachObject; }

private:
	void RefreshList();
	void OnItemFocused(ListItem *item);
	const char *m_Current;
	UIItemListForm *m_SpawnList;
	float m_selPercent;
	bool m_AttachObject;
};