#include "xrUICore/SpinBox/UICustomSpin.h"

class XRUICORE_API CUISpinNum : public CUICustomSpin
{
public:
    CUISpinNum();

    virtual void InitSpin(Fvector2 pos, Fvector2 size);

    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    virtual void OnBtnUpClick();
    virtual void OnBtnDownClick();

    void SetMax(int max) { m_iMax = max; };
    void SetMin(int min) { m_iMin = min; };
    int Value() const { return m_iVal; }
protected:
    void SetValue(int v);
    virtual bool CanPressUp();
    virtual bool CanPressDown();
    virtual void IncVal();
    virtual void DecVal();

    int m_iMax;
    int m_iMin;
    int m_iStep;
    int m_iVal;
    int m_opt_backup_value;
};

class XRUICORE_API CUISpinFlt : public CUICustomSpin
{
public:
    CUISpinFlt();

    virtual void InitSpin(Fvector2 pos, Fvector2 size);

    // CUIOptionsItem
    virtual void SetCurrentOptValue(); // opt->current
    virtual void SaveBackUpOptValue(); // current->backup
    virtual void SaveOptValue(); // current->opt
    virtual void UndoOptValue(); // backup->current
    virtual bool IsChangedOptValue() const; // backup!=current

    virtual void OnBtnUpClick();
    virtual void OnBtnDownClick();

    void SetMax(float max);
    void SetMin(float min);

protected:
    void SetValue(float v);
    virtual bool CanPressUp();
    virtual bool CanPressDown();
    virtual void IncVal();
    virtual void DecVal();

    float m_fMax;
    float m_fMin;
    float m_fStep;
    float m_fVal;
    float m_opt_backup_value;
};
