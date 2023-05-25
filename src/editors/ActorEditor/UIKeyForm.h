#pragma once
class UIKeyForm : public XrUI
{
public:
	UIKeyForm();
	virtual ~UIKeyForm();
	virtual void Draw();
	IC bool AutoChange() const { return m_AutoChange; }
	IC float Position() const { return m_Position; };

private:
	void SetMark(int id, int action);
	void DrawMark(int id);
	xr_vector<float> m_TempForPlotHistogram;
	float m_Position;
	bool m_AutoChange;
	float m_TimeFactor;
	CSMotion *m_currentEditMotion;
};
