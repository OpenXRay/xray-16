#pragma once
typedef fastdelegate::FastDelegate0<> TOnRenderContextMenu;
class ECORE_API UIRenderForm : public XrUI
{
public:
	UIRenderForm();
	virtual ~UIRenderForm();
	virtual void Draw();
	IC Ivector2 GetMousePos() const { return m_mouse_position; }
	IC void SetContextMenuEvent(TOnRenderContextMenu e) { m_OnContextMenu = e; }

private:
	Ivector2 m_mouse_position;
	ref_texture m_rt;
	TOnRenderContextMenu m_OnContextMenu;
	bool m_mouse_down;
	bool m_mouse_move;
	bool m_shiftstate_down;
};