#pragma once
class ESceneShapeTool;
class UIShapeTool : public UIToolCustom
{
public:
	ESceneShapeTool *Tool;
	UIShapeTool();
	virtual ~UIShapeTool();
	virtual void Draw();
	IC bool IsAttachShape() const { return m_AttachShape; }
	IC bool IsSphereMode() const { return m_SphereMode; }
	bool EditLevelBound;
	IC void SetSphereMode(bool mode) { m_SphereMode = mode; }
	IC void SetAttachShape(bool mode) { m_AttachShape = mode; }

private:
	bool m_AttachShape;
	bool m_SphereMode;
};