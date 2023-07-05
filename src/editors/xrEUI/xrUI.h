#pragma once
class XREUI_API xrUI
{
public:
	xrUI() : bOpen(true) {}
	enum FUI
	{
		F_NoDelete = 1,
	};
	Flags32 Flags;
	virtual ~xrUI();
	/*сдюкхрэ еякх ме опхцндхряъ*/
	virtual void ResetBegin();
	virtual void ResetEnd();
	bool IsClosed() const { return !bOpen; }
	virtual void Draw() = 0;

protected:
	bool bOpen;
};
