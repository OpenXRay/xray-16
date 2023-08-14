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

    auto ui = xr_new<CUIWindow>(ui_path);
    if (!CUIXmlInit::InitWindow(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
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

    string256 temp;
    xr_sprintf(temp, "%s (%d)", ui_path, index);
    auto ui = xr_new<CUIStatic>(temp);
    if (!CUIXmlInit::InitStatic(xml, ui_path, index, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIScrollView* UIHelper::CreateScrollView(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical /*= true*/)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIScrollView>();
    if (!CUIXmlInit::InitScrollView(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUITextWnd* UIHelper::CreateTextWnd(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUITextWnd>();
    if (!CUIXmlInit::InitTextWnd(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIEditBox* UIHelper::CreateEditBox(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIEditBox>();
    if (!CUIXmlInit::InitEditBox(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
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
        xr_delete(ui);
    }
    if (ui && parent)
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
        xr_delete(ui);
    }
    if (ui && parent)
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

    auto ui = xr_new<CUIFrameLineWnd>(ui_path);
    if (!CUIXmlInit::InitFrameLine(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
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
    if (!CUIXmlInit::InitTextFrameLine(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
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

    auto ui = xr_new<CUIFrameWindow>(ui_path);
    if (!CUIXmlInit::InitFrameWindow(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
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
    if (!CUIXmlInit::Init3tButton(xml, ui_path, index, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUICheckButton* UIHelper::CreateCheck(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUICheckButton>();
    if (!CUIXmlInit::InitCheck(xml, ui_path, 0, ui, critical) && !critical)
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
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
    if (!CUIXmlInit::InitDragDropListEx(xml, ui_path, index, ui) && !critical) // XXX: doesn't support 'critical' param, crashes if xml node is missing
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}

CUIDragDropReferenceList* UIHelper::CreateDragDropReferenceList(CUIXml& xml, LPCSTR ui_path, CUIWindow* parent, bool critical)
{
    // If it's not critical element, then don't crash if it doesn't exist
    if (!critical && !xml.NavigateToNode(ui_path, 0))
        return nullptr;

    auto ui = xr_new<CUIDragDropReferenceList>();
    if (!CUIXmlInit::InitDragDropListEx(xml, ui_path, 0, ui) && !critical) // XXX: doesn't support 'critical' param, crashes if xml node is missing
    {
        xr_delete(ui);
    }
    if (ui && parent)
    {
        parent->AttachChild(ui);
        ui->SetAutoDelete(true);
    }
    return ui;
}
