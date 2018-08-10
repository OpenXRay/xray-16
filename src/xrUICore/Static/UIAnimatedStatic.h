#pragma once
#include "UIStatic.h"

class XRUICORE_API CUIAnimatedStatic : public CUIStatic
{
    typedef CUIStatic inherited;
    // Количекство кадров анимации
    u32 m_uFrameCount;
    // Текущий фрейм
    u32 m_uCurFrame;
    // Размеры текстуры с анимацией в кадрах
    u32 m_uAnimRows, m_uAnimCols;
    // Размеры кадра на тектуре
    float m_uFrameWidth, m_uFrameHeight;
    // Время показа всей анимации в ms.
    u32 m_uAnimationDuration;
    // Время прошедшее с начала анимации
    u32 m_uTimeElapsed;
    // флаг-признак необходимости пересчета статичных параметров анимации
    bool m_bParamsChanged;
    // Признак проигрывания анимации
    bool m_bPlaying;

    Fvector2 m_pos;

    u32 m_prevTime;

    // Инициализация первого кадра
    // Params:	frameNum	- номер кадра: [0..m_uFrameCount)
    void SetFrame(const u32 frameNum);

public:
    CUIAnimatedStatic();

    // Устанавливаем параметры
    void SetOffset(float x, float y) { m_pos.set(x, y); };
    void SetFramesCount(u32 frameCnt)
    {
        m_uFrameCount = frameCnt;
        m_bParamsChanged = true;
    }
    void SetAnimCols(u32 animCols)
    {
        m_uAnimCols = animCols;
        m_bParamsChanged = true;
    }
    void SetAnimationDuration(u32 animDur)
    {
        m_uAnimationDuration = animDur;
        m_bParamsChanged = true;
    }
    void SetFrameDimentions(float frameW, float frameH)
    {
        m_uFrameHeight = frameH;
        m_uFrameWidth = frameW;
        m_bParamsChanged = true;
    }
    // Управление
    void Play()
    {
        m_bPlaying = true;
        m_prevTime = Device.dwTimeContinual;
    }
    void Stop() { m_bPlaying = false; }
    void Rewind(u32 delta = 0)
    {
        m_uCurFrame = 0xffffffff;
        m_uTimeElapsed = delta;
    }
    void SetAnimPos(float pos);
    // Флаг-признак циклического проигрывания
    bool m_bCyclic;

    virtual void Update();
};
