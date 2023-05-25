#include "stdafx.h"

UITopBarForm::UITopBarForm()
{

#define ADD_BUTTON_IMAGE_T1(Class, Name)
#define ADD_BUTTON_IMAGE_T2(Class, Name)
#define ADD_BUTTON_IMAGE_S(Name)                                      \
	m_t##Name = EDevice.Resources->_CreateTexture("ed\\bar\\" #Name); \
	m_t##Name->Load();                                                \
	m_time##Name = 0;
#define ADD_BUTTON_IMAGE_D(Name)                                      \
	m_t##Name = EDevice.Resources->_CreateTexture("ed\\bar\\" #Name); \
	m_t##Name->Load();                                                \
	m_b##Name = false;
#include "UITopBarForm_ButtonList.h"
	RefreshBar();
}

UITopBarForm::~UITopBarForm()
{
}

void UITopBarForm::Draw()
{
	ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x, viewport->Pos.y + UI->GetMenuBarHeight()));
	ImGui::SetNextWindowSize(ImVec2(viewport->Size.x, UIToolBarSize));
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = 0 | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(2, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 2));
	ImGui::Begin("TOOLBAR", NULL, window_flags);
	{
#define ADD_BUTTON_IMAGE_S(Name)                                                                                                                                                             \
	if (ImGui::ImageButton(m_t##Name->surface_get(), ImVec2(20, 20), ImVec2(m_time##Name > EDevice.TimerAsync() ? 0.5 : 0, 0), ImVec2(m_time##Name > EDevice.TimerAsync() ? 1 : 0.5, 1), 0)) \
	{                                                                                                                                                                                        \
		m_time##Name = EDevice.TimerAsync() + 130;                                                                                                                                           \
		Click##Name();                                                                                                                                                                       \
	}                                                                                                                                                                                        \
	ImGui::SameLine();
#define ADD_BUTTON_IMAGE_D(Name)                                                                                                         \
	if (ImGui::ImageButton(m_t##Name->surface_get(), ImVec2(20, 20), ImVec2(m_b##Name ? 0.5 : 0, 0), ImVec2(m_b##Name ? 1 : 0.5, 1), 0)) \
	{                                                                                                                                    \
		m_b##Name = !m_b##Name;                                                                                                          \
		Click##Name();                                                                                                                   \
	}                                                                                                                                    \
	ImGui::SameLine();
#define ADD_BUTTON_IMAGE_P(Name)                                                                                                         \
	if (ImGui::ImageButton(m_t##Name->surface_get(), ImVec2(20, 20), ImVec2(m_b##Name ? 0.5 : 0, 0), ImVec2(m_b##Name ? 1 : 0.5, 1), 0)) \
	{                                                                                                                                    \
		Click##Name();                                                                                                                   \
	}                                                                                                                                    \
	ImGui::SameLine();
#define ADD_BUTTON_IMAGE_T1(Class, Name)                                 \
	ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 0)); \
	if (ImGui::Button("" #Name, ImVec2(20, 20)))                         \
	{                                                                    \
		Click##Class##Name();                                            \
	}                                                                    \
	ImGui::SameLine();                                                   \
	ImGui::PopStyleColor(1);
#define ADD_BUTTON_IMAGE_T1(Class, Name)                                 \
	ImGui::PushID("" #Class);                                            \
	ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor::HSV(0, 0, 0)); \
	if (ImGui::Button("" #Name, ImVec2(20, 20)))                         \
	{                                                                    \
		Click##Class##Name();                                            \
	}                                                                    \
	ImGui::SameLine();                                                   \
	ImGui::PopStyleColor(1);                                             \
	ImGui::PopID();
#define ADD_BUTTON_IMAGE_T2(Class, Name)         \
	ImGui::PushID("" #Class);                    \
	if (ImGui::Button("" #Name, ImVec2(20, 20))) \
	{                                            \
		Click##Class##Name();                    \
	}                                            \
	ImGui::SameLine();                           \
	ImGui::PopID();
#include "UITopBarForm_ButtonList.h"
		bool Simulate = ATools->IsPhysics();

		if (ImGui::Checkbox("Simulate", &Simulate))
		{
			if (Simulate)
				ATools->PhysicsSimulate();
			else
				ATools->PhysicsStopSimulate();
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(5);
}
void UITopBarForm::RefreshBar()
{
	{
		m_bSelect = false;
		m_bAdd = false;
		m_bMove = false;
		m_bRotate = false;
		m_bScale = false;

		switch (Tools->GetAction())
		{
		case etaSelect:
			m_bSelect = true;
			break;
		case etaAdd:
			m_bAdd = true;
			break;
		case etaMove:
			m_bMove = true;
			break;
		case etaRotate:
			m_bRotate = true;
			break;
		case etaScale:
			m_bScale = true;
			break;
		default:
			THROW;
		}
	}
	{
		m_bX = false;
		m_bY = false;
		m_bZ = false;
		m_bZX = false;
		// axis
		switch (Tools->GetAxis())
		{
		case etAxisX:
			m_bX = true;
			break;
		case etAxisY:
			m_bY = true;
			break;
		case etAxisZ:
			m_bZ = true;
			break;
		case etAxisZX:
			m_bZX = true;
			break;
		default:
			THROW;
		}
	}
	// settings
	m_bCsLocal = Tools->GetSettings(etfCSParent);
	m_bNuScale = Tools->GetSettings(etfNUScale);
	m_bNSnap = Tools->GetSettings(etfNormalAlign);
	m_bGSnap = Tools->GetSettings(etfGSnap);
	m_bOSnap = Tools->GetSettings(etfOSnap);
	m_bMoveToSnap = Tools->GetSettings(etfMTSnap);
	m_bVSnap = Tools->GetSettings(etfVSnap);
	m_bASnap = Tools->GetSettings(etfASnap);
	m_bMSnap = Tools->GetSettings(etfMSnap);
}

void UITopBarForm::ClickUndo()
{
	ExecCommand(COMMAND_UNDO);
}

void UITopBarForm::ClickRedo()
{
	ExecCommand(COMMAND_REDO);
}
void UITopBarForm::ClickZoom()
{
	ExecCommand(COMMAND_ZOOM_EXTENTS, FALSE);
}

void UITopBarForm::ClickZoomSel()
{
	ExecCommand(COMMAND_ZOOM_EXTENTS, TRUE);
}
void UITopBarForm::ClickSelect()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etaSelect);
	m_bSelect = true;
	m_bAdd = false;
	m_bMove = false;
	m_bRotate = false;
	m_bScale = false;
}
void UITopBarForm::ClickAdd()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etaAdd);
	m_bSelect = false;
	m_bAdd = true;
	m_bMove = false;
	m_bRotate = false;
	m_bScale = false;
}
void UITopBarForm::ClickMove()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etaMove);
	m_bSelect = false;
	m_bAdd = false;
	m_bMove = true;
	m_bRotate = false;
	m_bScale = false;
}
void UITopBarForm::ClickRotate()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etaRotate);
	m_bSelect = false;
	m_bAdd = false;
	m_bMove = false;
	m_bRotate = true;
	m_bScale = false;
}
void UITopBarForm::ClickScale()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etaScale);
	m_bSelect = false;
	m_bAdd = false;
	m_bMove = false;
	m_bRotate = false;
	m_bScale = true;
}
void UITopBarForm::ClickX()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etAxisX);
	m_bX = true;
	m_bY = false;
	m_bZ = false;
	m_bZX = false;
}
void UITopBarForm::ClickY()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etAxisY);
	m_bX = false;
	m_bY = true;
	m_bZ = false;
	m_bZX = false;
}
void UITopBarForm::ClickZ()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etAxisZ);
	m_bX = false;
	m_bY = false;
	m_bZ = true;
	m_bZX = false;
}
void UITopBarForm::ClickZX()
{
	ExecCommand(COMMAND_CHANGE_ACTION, etAxisZX);
	m_bX = false;
	m_bY = false;
	m_bZ = false;
	m_bZX = true;
}

void UITopBarForm::ClickCsLocal() { ExecCommand(COMMAND_SET_SETTINGS, etfCSParent, m_bCsLocal); }
void UITopBarForm::ClickNuScale() { ExecCommand(COMMAND_SET_SETTINGS, etfNUScale, m_bNuScale); }
void UITopBarForm::ClickGSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfGSnap, m_bGSnap); }
void UITopBarForm::ClickOSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfOSnap, m_bOSnap); }
void UITopBarForm::ClickMoveToSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfMTSnap, m_bMoveToSnap); }
void UITopBarForm::ClickNSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfNormalAlign, m_bNSnap); }
void UITopBarForm::ClickVSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfVSnap, m_bVSnap); }
void UITopBarForm::ClickASnap() { ExecCommand(COMMAND_SET_SETTINGS, etfASnap, m_bASnap); }
void UITopBarForm::ClickMSnap() { ExecCommand(COMMAND_SET_SETTINGS, etfMSnap, m_bMSnap); }

void UITopBarForm::ClickCameraP()
{
	EDevice.m_Camera.SetStyle(csPlaneMove);
	UI->RedrawScene();
}
void UITopBarForm::ClickCameraA()
{
	EDevice.m_Camera.SetStyle(cs3DArcBall);
	UI->RedrawScene();
}
void UITopBarForm::ClickCameraF()
{
	EDevice.m_Camera.SetStyle(csFreeFly);
	UI->RedrawScene();
}

void UITopBarForm::ClickViewB1()
{
	EDevice.m_Camera.ViewBack();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewB2()
{
	EDevice.m_Camera.ViewBottom();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewF()
{
	EDevice.m_Camera.ViewFront();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewL()
{
	EDevice.m_Camera.ViewLeft();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewR()
{
	EDevice.m_Camera.ViewRight();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewT()
{
	EDevice.m_Camera.ViewTop();
	UI->RedrawScene();
}
void UITopBarForm::ClickViewX()
{
	EDevice.m_Camera.ViewReset();
	UI->RedrawScene();
}