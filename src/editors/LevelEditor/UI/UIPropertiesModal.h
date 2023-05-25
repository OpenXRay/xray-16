#pragma once
class UIPropertiesModal : public XrUI
{
public:
	UIPropertiesModal();
	virtual ~UIPropertiesModal();
	virtual void Draw();
	static void Update();
	static bool GetResult(bool &ok);
	static void Show(PropItemVec &items);
	static IC UIPropertiesForm *GetProperties() { return Form->m_Props; }

private:
	static UIPropertiesModal *Form;

private:
	UIPropertiesForm *m_Props;

private:
	enum Result
	{
		R_Ok,
		R_Cancel
	};
	Result m_Result;
};