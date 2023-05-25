#pragma once
#include "..\XrEUI\stdafx.h"
class UITest : public XrUI
{
public:
	UITest();
	virtual ~UITest();
	virtual void Draw();

private:
	string1024 out;
};
