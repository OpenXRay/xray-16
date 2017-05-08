#pragma once //--#SM+#--

#include "xrCore/_quaternion.h"

namespace KinematicsABT
{
struct additional_bone_transform // Описывает постоянное смещение для указанной кости --#SM+#--
{
    bool m_bRotGlobal; // Use XYZ axis instead of HPB
    u16 m_bone_id; // Bone ID
    Fmatrix m_transform; // Rotation + Position

    additional_bone_transform() : m_bRotGlobal(false), m_bone_id(-1) { m_transform.identity(); }

    // Установить поворот кости (статичные оси)
    void setRotGlobal(const float _x, const float _y, const float _z)
    {
        m_bRotGlobal = true;
        m_transform.setXYZ(_x, _y, _z);
    }

    void setRotGlobal(Fvector& vRotOfs)
    {
        setRotGlobal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить поворот кости (кость крутится вместе с осями)
    void setRotLocal(const float _x, const float _y, const float _z)
    {
        m_bRotGlobal = false;
        Fquaternion Q;
        Q.rotationYawPitchRoll(_y, _x, _z);
        m_transform.rotation(Q);
    }

    void setRotLocal(Fvector& vRotOfs)
    {
        setRotLocal(vRotOfs.x, vRotOfs.y, vRotOfs.z);
    }

    // Установить смещение позиции кости
    void setPosOffset(const float _x, const float _y, const float _z)
    {
        m_transform.translate_over(_x, _y, _z);
    }

    void setPosOffset(Fvector& vPosOfs)
    {
        setPosOffset(vPosOfs.x, vPosOfs.y, vPosOfs.z);
    }
};
}
