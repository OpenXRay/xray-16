////////////////////////////////////////////////////////////////////////////
//	Module 		: UIHelper.cpp
//	Created 	: 17.01.2008
//	Author		: Evgeniy Sokolov
//	Description : UI Helper class implementation
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "UIHelper.h"
#include "UIXmlInit.h"

#include "xrUICore/ProgressBar/UIProgressBar.h"
#include "xrUICore/ProgressBar/UIProgressShape.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"
#include "xrUICore/Windows/UITextFrameLineWnd.h"
#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Buttons/UI3tButton.h"
#include "xrUICore/Buttons/UICheckButton.h"
#include "xrUICore/Hint/UIHint.h"
#include "UIDragDropReferenceList.h"
#include "xrUICore/EditBox/UIEditBox.h"

CUIWindow* UIHelper::CreateNormalWindow(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIWindow>();
    if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    CUIXmlInit::InitWindow(xml, ui_path, 0, ui);
    return ui;
}

CUIStatic* UIHelper::CreateStatic(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    return CreateStatic(xml, ui_path, 0, parent, critical);
}

CUIStatic* UIHelper::CreateStatic(CUIXml& xml, LPCSTR ui_path, int index, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, index))
        return nullptr;

    auto ui = xr_new<CUIStatic>();
    if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    CUIXmlInit::InitStatic(xml, ui_path, index, ui);
    return ui;
}

CUIScrollView* UIHelper::CreateScrollView(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIScrollView>();
    if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    CUIXmlInit::InitScrollView(xml, ui_path, 0, ui);
    return ui;
}

CUITextWnd* UIHelper::CreateTextWnd(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUITextWnd>();
    if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    CUIXmlInit::InitTextWnd(xml, ui_path, 0, ui);
    return ui;
}

CUIEditBox* UIHelper::CreateEditBox(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIEditBox>();
    if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    CUIXmlInit::InitEditBox(xml, ui_path, 0, ui);
    return ui;
}

CUIProgressBar* UIHelper::CreateProgressBar(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIProgressBar>();
    if (!CUIXmlInit::InitProgressBar(xml, ui_path, 0, ui))
    {
        R_ASSERT2(!critical, "Failed to create progress bar");
        xr_delete(ui);
    }
    else if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIProgressShape* UIHelper::CreateProgressShape(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIProgressShape>();
    if (!CUIXmlInit::InitProgressShape(xml, ui_path, 0, ui))
    {
        R_ASSERT2(!critical, "Failed to create progress shape");
        xr_delete(ui);
    }
    else if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIFrameLineWnd* UIHelper::CreateFrameLine(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIFrameLineWnd>();
    if (!CUIXmlInit::InitFrameLine(xml, ui_path, 0, ui, critical))
    {
        R_ASSERT2(!critical, "Failed to create frame line");
        xr_delete(ui);
    }
    else if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUITextFrameLineWnd* UIHelper::CreateTextFrameLine(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUITextFrameLineWnd>();
    if (!CUIXmlInit::InitTextFrameLine(xml, ui_path, 0, ui, critical))
    {
        R_ASSERT2(!critical, "Failed to create frame line");
        xr_delete(ui);
    }
    else if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIFrameWindow* UIHelper::CreateFrameWindow(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIFrameWindow>();
    if (!CUIXmlInit::InitFrameWindow(xml, ui_path, 0, ui, critical))
    {
        R_ASSERT2(!critical, "Failed to create frame window");
        xr_delete(ui);
    }
    else if (parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUI3tButton* UIHelper::Create3tButton(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    return Create3tButton(xml, ui_path, 0, parent, critical);
}

CUI3tButton* UIHelper::Create3tButton(CUIXml& xml, LPCSTR ui_path, int index, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, index))
        return nullptr;

    auto ui = xr_new<CUI3tButton>();
    parent->AttachChild(ui);
    ui->SetAutoDelete(true);
    CUIXmlInit::Init3tButton(xml, ui_path, index, ui);
    return ui;
}

CUICheckButton* UIHelper::CreateCheck(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUICheckButton>();
    parent->AttachChild(ui);
    ui->SetAutoDelete(true);
    CUIXmlInit::InitCheck(xml, ui_path, 0, ui);
    return ui;
}

UIHint* UIHelper::CreateHint(CUIXml& xml, LPCSTR ui_path /*, CUIWindow* parent*/, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<UIHint>();
    ui->SetAutoDelete(true);
    ui->init_from_xml(xml, ui_path);
    return ui;
}

CUIDragDropListEx* UIHelper::CreateDragDropListEx(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    return CreateDragDropListEx(xml, ui_path, 0, parent, critical);
}

CUIDragDropListEx* UIHelper::CreateDragDropListEx(CUIXml& xml, LPCSTR ui_path, int index, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, index))
        return nullptr;

    auto ui = xr_new<CUIDragDropListEx>();
    parent->AttachChild(ui);
    ui->SetAutoDelete(true);
    CUIXmlInit::InitDragDropListEx(xml, ui_path, index, ui);
    return ui;
}

CUIDragDropReferenceList* UIHelper::CreateDragDropReferenceList(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIDragDropReferenceList>();
    parent->AttachChild(ui);
    ui->SetAutoDelete(true);
    CUIXmlInit::InitDragDropListEx(xml, ui_path, 0, ui);
    return ui;
}
