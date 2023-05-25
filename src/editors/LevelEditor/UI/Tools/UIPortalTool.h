#pragma once
class EScenePortalTool;
class UIPortalTool : public UIToolCustom
{
public:
	UIPortalTool();
	virtual ~UIPortalTool();
	virtual void Draw();
	EScenePortalTool *tool;
};