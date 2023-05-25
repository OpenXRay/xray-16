#pragma once
enum TShiftState_
{
	ssNone = 0,
	ssShift = 1,
	ssLeft = 2,
	ssRight = 4,
	ssCtrl = 8,
	ssAlt = 16,
};
using TShiftState = int;
constexpr int UIToolBarSize = 24;
class XREUI_API XrUIManager
{
public:
	XrUIManager();
	void Push(XrUI *ui, bool need_deleted = true);
	void Draw();

	virtual ~XrUIManager();

	LRESULT WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void Initialize(HWND hWnd, IDirect3DDevice9 *device, const char *ini_path);
	void Destroy();

	void ResetBegin();
	void ResetEnd();
	virtual bool ApplyShortCut(DWORD Key, TShiftState Shift) = 0;

	inline float GetMenuBarHeight() const { return m_MenuBarHeight; }
	inline TShiftState GetShiftState() const { return m_ShiftState; };

protected:
	virtual void OnDrawUI();

private:
	float m_MenuBarHeight;
	void ApplyShortCut(DWORD Key);
	TShiftState m_ShiftState;
	xr_vector<XrUI *> m_UIArray;
	string_path m_name_ini;
};
