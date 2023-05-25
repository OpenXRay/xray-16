#pragma once
class ESceneFogVolumeTool;
class UIFogVolTool : public UIToolCustom
{
public:
	UIFogVolTool();
	virtual ~UIFogVolTool();
	virtual void Draw();
	ESceneFogVolumeTool *ParentTools;
};