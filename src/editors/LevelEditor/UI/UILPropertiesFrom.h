#pragma once
class UILPropertiesFrom : public XrUI
{
public:
	UILPropertiesFrom();
	virtual ~UILPropertiesFrom();
	virtual void Draw();
	IC void Open() { bOpen = true; }
	IC void Close() { bOpen = false; }
};