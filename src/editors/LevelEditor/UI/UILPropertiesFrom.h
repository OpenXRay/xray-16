#pragma once
class UILPropertiesFrom : public xrUI
{
public:
	UILPropertiesFrom();
	virtual ~UILPropertiesFrom();
	virtual void Draw();
	IC void Open() { bOpen = true; }
	IC void Close() { bOpen = false; }
};