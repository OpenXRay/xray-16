#pragma once
#include "..\xrEUI\stdafx.h"
class UITest : public xrUI
{
public:
	UITest();
	virtual ~UITest();
	virtual void Draw();

private:
	string1024 out;
};
