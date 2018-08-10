// File:        UIInteractiveBackground.h
// Description: template class designed for UI controls to represent their state;
//              there are 4 states: Enabled, Disabled, Hightlighted and Touched.
//              As a rule you can use one of 3 background types:
//              Normal Texture, String Texture, Frame Texture (CUIStatic, CUIFrameLineWnd, CUIFrameWindow)
// Created:     29.12.2004
// Author:      Serhiy 0. Vynnychenko
// Mial:        narrator@gsc-game.kiev.ua
//
// Copyright 2004 GSC Game World
//

#pragma once

#include "xrUICore/Windows/UIFrameWindow.h"
#include "xrUICore/Windows/UIWindow.h"
#include "xrUICore/Windows/UIFrameLineWnd.h"

enum IBState
{
    S_Enabled = 0,
    S_Disabled,
    S_Highlighted,
    S_Touched,
    S_Current,
    S_Total
};

template <class T>
class CUIInteractiveBackground : public CUIWindow
{
public:
    CUIInteractiveBackground();
    virtual ~CUIInteractiveBackground(){};

    void InitIB(Fvector2 pos, Fvector2 size);
    void InitIB(LPCSTR texture_e, Fvector2 pos, Fvector2 size);
    T* Get(IBState state) { return m_states[state]; };
    void InitState(IBState state, LPCSTR texture);
    void SetCurrentState(IBState state);

    virtual void Draw();
    virtual void SetWidth(float width);
    virtual void SetHeight(float heigth);

protected:
    T* m_states[S_Total];
};

template <class T>
CUIInteractiveBackground<T>::CUIInteractiveBackground()
{
    ZeroMemory(m_states, S_Total * sizeof(T*));
}

template <class T>
void CUIInteractiveBackground<T>::InitIB(Fvector2 pos, Fvector2 size)
{
    CUIWindow::SetWndPos(pos);
    CUIWindow::SetWndSize(size);
}

template <class T>
void CUIInteractiveBackground<T>::InitIB(LPCSTR texture, Fvector2 pos, Fvector2 size)
{
    CUIWindow::SetWndPos(pos);
    CUIWindow::SetWndSize(size);

    InitState(S_Enabled, texture);
}

template <class T>
void CUIInteractiveBackground<T>::InitState(IBState state, LPCSTR texture)
{
    Fvector2 size = GetWndSize();

    if (!m_states[state])
    {
        m_states[state] = new T();
        m_states[state]->SetAutoDelete(true);
        AttachChild(m_states[state]);
    }

    m_states[state]->InitTexture(texture);
    m_states[state]->SetWndPos(Fvector2().set(0, 0));
    m_states[state]->SetWndSize(size);

    SetCurrentState(state);
}

template <class T>
void CUIInteractiveBackground<T>::SetCurrentState(IBState state)
{
    m_states[S_Current] = m_states[state];
    if (!m_states[S_Current])
        m_states[S_Current] = m_states[S_Enabled];
}

template <class T>
void CUIInteractiveBackground<T>::Draw()
{
    if (m_states[S_Current])
        m_states[S_Current]->Draw();
}

template <class T>
void CUIInteractiveBackground<T>::SetWidth(float width)
{
    for (int i = 0; i < S_Total; ++i)
        if (m_states[i])
            m_states[i]->SetWidth(width);
}

template <class T>
void CUIInteractiveBackground<T>::SetHeight(float height)
{
    for (int i = 0; i < S_Total; ++i)
        if (m_states[i])
            m_states[i]->SetHeight(height);
}

typedef CUIInteractiveBackground<CUIFrameLineWnd> CUI_IB_FrameLineWnd;
