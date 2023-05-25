#pragma once
class UITopBarForm : public XrUI
{
public:
	UITopBarForm();
	virtual ~UITopBarForm();
	virtual void Draw();
	void RefreshBar();

private:
#define ADD_BUTTON_IMAGE_T1(Class, Name) void Click##Class##Name();
#define ADD_BUTTON_IMAGE_T2(Class, Name) void Click##Class##Name();
#define ADD_BUTTON_IMAGE_S(Name) \
	void Click##Name();          \
	ref_texture m_t##Name;       \
	u32 m_time##Name;
#define ADD_BUTTON_IMAGE_D(Name) \
	void Click##Name();          \
	ref_texture m_t##Name;       \
	bool m_b##Name;
#define ADD_BUTTON_IMAGE_P(Name) \
	void Click##Name();          \
	ref_texture m_t##Name;       \
	bool m_b##Name;
#include "UITopBarForm_ButtonList.h"
};