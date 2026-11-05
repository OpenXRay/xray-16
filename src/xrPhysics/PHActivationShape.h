#pragma once

#include "PHValideValues.h"
#include "PHObject.h"
#include "xrPhysicsCore/IPhysicsCore.h"

class IPhysicsShellHolder;

class CPHActivationShape : public CPHObject
{
    CharacterVirtualHandle m_char_handle = INVALID_CHARACTER_VIRTUAL_HANDLE;
    PhysicsShapeHandle m_geom = nullptr;
    Flags16 m_flags;
    CSafeFixedRotationState m_safe_state;
    Fmatrix m_transform = Fidentity;
    Fvector m_size = Fvector().set(0, 0, 0);
    IPhysicsShellHolder* m_ref_object = nullptr;
    int m_type = 0;
    bool m_check_characters = true;

#ifdef DEBUG
    virtual IPhysicsShellHolder* ref_object();
#endif

public:
    enum EType
    {
        etBox,
        etCylinder,
        etSphere
    };

    enum
    {
        flFixedRotation = 1 << 0,
        flFixedPosition = 1 << 1,
        flStaticEnvironment = 1 << 2,
        flGravity = 1 << 3
    };

    CPHActivationShape();
    ~CPHActivationShape();

    void Create(const Fvector start_pos, const Fvector start_size, IPhysicsShellHolder* ref_obj, EType type = etBox,
        u16 flags = 0);
    void Destroy();
    bool Activate(
        const Fvector need_size, u16 steps, float max_displacement, float max_rotation, bool un_freeze_later = false);

    const Fvector& Position();
    void Size(Fvector& size);

    // Заменяем dBodyID ODEBody()
    CharacterVirtualHandle GetCharacterVirtualHandle() { return m_char_handle; }

    void set_rotation(const Fmatrix& rot);
    void CheckCharacters(bool enabled) { m_check_characters = enabled; }

private:
    // dReal -> float
    virtual void PhDataUpdate(float step);
    virtual void PhTune(float step);
    virtual void CutVelocity(float l_limit, float a_limit);

    // Обновленная сигнатура контакта (dContact -> нормаль и глубина)
    virtual void InitContact(bool& do_collide, bool bo1, float depth, CPhysicsGeom* my_geom, CPhysicsGeom* oposite_geom, u16 material_idx_1, u16 material_idx_2);

    // dGeomID -> PhysicsShapeHandle
    virtual PhysicsShapeHandle dSpacedGeom();

    virtual void get_spatial_params();
    virtual u16 get_elements_number() { return 0; }
    virtual CPHSynchronize* get_element_sync(u16 element) { return NULL; }
};
