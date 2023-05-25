#pragma once
class UILeftBarForm : public XrUI
{
public:
	UILeftBarForm();
	virtual ~UILeftBarForm();
	virtual void Draw();
	IC bool IsUseSnapList() const { return m_UseSnapList; }
	IC bool IsSnapListMode() const { return m_SnapListMode; }

private:
	bool m_UseSnapList;
	bool m_SnapListMode;
	int m_SnapItem_Current;
};
