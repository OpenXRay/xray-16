#pragma once
class EDetailManager;
class UIDOTool : public UIToolCustom
{
public:
	UIDOTool();
	virtual ~UIDOTool();
	virtual void Draw();
	virtual void OnDrawUI();
	EDetailManager *DM;

private:
	bool m_DOShuffle;
};