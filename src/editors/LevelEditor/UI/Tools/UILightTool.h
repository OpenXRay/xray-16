#pragma once
class UILightTool : public UIToolCustom
{
public:
	UILightTool();
	virtual ~UILightTool();
	virtual void Draw();

private:
	void UseInD3D(bool bAll, bool bFlag);
};