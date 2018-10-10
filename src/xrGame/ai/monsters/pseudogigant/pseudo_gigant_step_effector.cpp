#include "StdAfx.h"
#include "pseudo_gigant_step_effector.h"

CPseudogigantStepEffector::CPseudogigantStepEffector(float time, float amp, float periods, float power)
    : CEffectorCam(eCEPseudoGigantStep, time)
{
    total = time;

    max_amp = amp * power;
    period_number = periods;
    this->power = power;
}

BOOL CPseudogigantStepEffector::ProcessCam(SCamEffectorInfo& info)
{
    fLifeTime -= Device.fTimeDelta;
    if (fLifeTime < 0)
        return FALSE;

    // процент оставшегося времени
    float time_left_perc = fLifeTime / total;

    // Инициализация
    Fmatrix Mdef;
    Mdef.identity();
    Mdef.j.set(info.n);
    Mdef.k.set(info.d);
    Mdef.i.crossproduct(info.n, info.d);
    Mdef.c.set(info.p);

    float period_all = period_number * PI_MUL_2; // макс. значение цикла
    float k = 1 - time_left_perc + EPS_L + (1 - power);
    float cur_amp = max_amp * (PI / 180) / (10 * k * k);

    Fvector dangle;
    dangle.x = cur_amp / 2 * _sin(period_all * (1.0f - time_left_perc));
    dangle.y = cur_amp * _cos(period_all / 2 * (1.0f - time_left_perc));
    dangle.z = cur_amp / 4 * _sin(period_all / 4 * (1.0f - time_left_perc));

    // Установить углы смещения
    Fmatrix R;
    R.setHPB(dangle.x, dangle.y, dangle.z);

    Fmatrix mR;
    mR.mul(Mdef, R);

    info.d.set(mR.k);
    info.n.set(mR.j);

    return TRUE;
}
