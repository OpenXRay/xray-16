#pragma once
class XREPROPS_API UINumericVectorForm : public XrUI
{
public:
	UINumericVectorForm(const char *title, Fvector *data, Fvector *Reset = 0, int decimal = 0, Fvector *Min = 0, Fvector *Max = 0);
	virtual ~UINumericVectorForm();
	virtual void Draw();

private:
	void CLBOk();
	void CLBCancel();
	void CLBReset();
	Fvector m_Edit;
	xr_string m_Title;
	Fvector *m_Reset;
	Fvector *m_Out;
	Fvector *m_Min;
	Fvector *m_Max;
	int m_Decimal;
};
