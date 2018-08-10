// file:		UISpinNum.cpp
// description:	Spin Button with numerical data (unlike text data)
// created:		15.06.2005
// author:		Serge Vynnychenko
//

#include "pch.hpp"
#include "UISpinNum.h"
#include "Lines/UILines.h"

CUISpinNum::CUISpinNum() : m_iVal(0), m_iMin(0), m_iMax(100), m_iStep(1) {}
void CUISpinNum::SetCurrentOptValue() // opt->current
{
    GetOptIntegerValue(m_iVal, m_iMin, m_iMax);
    SetValue(m_iVal);
}

void CUISpinNum::SaveBackUpOptValue() // current->backup
{
    m_opt_backup_value = m_iVal;
}

void CUISpinNum::SaveOptValue() // current->opt
{
    CUIOptionsItem::SaveOptValue();
    SaveOptIntegerValue(m_iVal);
}

void CUISpinNum::UndoOptValue() // backup->current
{
    m_iVal = m_opt_backup_value;
    SetValue(m_iVal);
    CUIOptionsItem::UndoOptValue();
}

bool CUISpinNum::IsChangedOptValue() const // backup!=current
{
    return m_iVal != m_opt_backup_value;
}

void CUISpinNum::InitSpin(Fvector2 pos, Fvector2 size)
{
    CUICustomSpin::InitSpin(pos, size);
    SetValue(m_iVal);
}

void CUISpinNum::IncVal()
{
    if (CanPressUp())
        m_iVal += m_iStep;

    SetValue(m_iVal);
}

void CUISpinNum::DecVal()
{
    if (CanPressDown())
        m_iVal -= m_iStep;

    SetValue(m_iVal);
}

void CUISpinNum::OnBtnUpClick()
{
    IncVal();
    CUICustomSpin::OnBtnUpClick();
}

void CUISpinNum::OnBtnDownClick()
{
    DecVal();
    CUICustomSpin::OnBtnDownClick();
}

void CUISpinNum::SetValue(int v)
{
    string16 buff;
    m_pLines->SetText(xr_itoa(v, buff, 10));
}

bool CUISpinNum::CanPressUp() { return m_iVal + m_iStep <= m_iMax; }
bool CUISpinNum::CanPressDown() { return m_iVal - m_iStep >= m_iMin; }
CUISpinFlt::CUISpinFlt() : m_fVal(0), m_fMin(0), m_fMax(100), m_fStep(0.1f) {}
void CUISpinFlt::SaveBackUpOptValue()
{
    m_opt_backup_value = m_fVal;
}

void CUISpinFlt::UndoOptValue()
{
    m_fVal = m_opt_backup_value;
    SetValue(m_fVal);
    CUIOptionsItem::UndoOptValue();
}

void CUISpinFlt::SetCurrentOptValue()
{
    GetOptFloatValue(m_fVal, m_fMin, m_fMax);
    SetValue(m_fVal);
}

void CUISpinFlt::SaveOptValue()
{
    CUIOptionsItem::SaveOptValue();
    SaveOptFloatValue(m_fVal);
}

bool CUISpinFlt::IsChangedOptValue() const { return !fsimilar(m_fVal, m_opt_backup_value); }
void CUISpinFlt::InitSpin(Fvector2 pos, Fvector2 size)
{
    CUICustomSpin::InitSpin(pos, size);
    SetValue(m_fVal);
}

void CUISpinFlt::IncVal()
{
    m_fVal += m_fStep;
    clamp(m_fVal, m_fMin, m_fMax);
    SetValue(m_fVal);
}

void CUISpinFlt::DecVal()
{
    m_fVal -= m_fStep;
    clamp(m_fVal, m_fMin, m_fMax);
    SetValue(m_fVal);
}

void CUISpinFlt::OnBtnUpClick()
{
    IncVal();

    CUICustomSpin::OnBtnUpClick();
}

void CUISpinFlt::OnBtnDownClick()
{
    DecVal();

    CUICustomSpin::OnBtnDownClick();
}

void CUISpinFlt::SetValue(float v)
{
    string16 buff;
    xr_sprintf(buff, "%.1f", v);
    m_pLines->SetText(buff);
}

bool CUISpinFlt::CanPressUp() { return m_fVal + m_fStep <= m_fMax; }
bool CUISpinFlt::CanPressDown() { return (m_fVal - m_fStep > m_fMin) || fsimilar(m_fVal - m_fStep, m_fMin); }
