#pragma once

#include "PHDefs.h"
#include "PHImpact.h"
#include "xrPhysicsCore/IPhysicsCore.h"
#include "PHJointDestroyInfo.h"

class CPHFracture;
class CPHElement;

struct SPhysicsJointFeedback;

using CFEEDBACK_STORAGE = xr_vector<SPhysicsJointFeedback>;

IC void sub_diapasones(u16& from1, u16& to1, const u16& from0, const u16& to0);

class CShellSplitInfo
{
    friend class CPHWorld;
    friend class CPHFracturesHolder;
    friend class CPHShellSplitterHolder;
    friend class CPHElement;
    bool HaveElements() { return m_end_el_num != m_start_el_num; }
    bool HaveJoints() { return m_start_jt_num != m_end_jt_num; }

public:
    void sub_diapasone(const CShellSplitInfo& sub)
    {
        sub_diapasones(m_start_el_num, m_end_el_num, sub.m_start_el_num, sub.m_end_el_num);
        sub_diapasones(m_start_jt_num, m_end_jt_num, sub.m_start_jt_num, sub.m_end_jt_num);
    }

protected:
    u16 m_start_el_num;
    u16 m_end_el_num;
    u16 m_start_jt_num;
    u16 m_end_jt_num;
    u16 m_start_geom_num;
    u16 m_end_geom_num;
    u16 m_bone_id;
};

class CPHFracture : public CShellSplitInfo
{
    friend class CPHWorld;
    friend class CPHFracturesHolder;
    friend class CPHElement;
    friend class CPHShell;
    bool m_breaked;

    float m_firstM;
    float m_secondM;
    PhysicsMassProperties m_first_properties, m_second_properties;
    Fvector m_first_center = {0, 0, 0}, m_second_center = {0, 0, 0};
    Fvector m_cached_center = {0, 0, 0};
    u32 m_cached_revision = u32(-1);
    u16 m_cached_geometry_count = 0;
    float m_cached_mass = -1;

    float m_break_force;
    float m_break_torque;
    Fvector m_pos_in_element;
    float m_add_torque_z;
    CPHFracture();

public:
    bool Update(CPHElement* element);
    bool Breaked() { return m_breaked; }

    void SetMassParts(float first, float second);
    void MassSetZerro();
    void MassAddToFirst(float m);
    void MassAddToSecond(float m);
    void MassSubFromFirst(float m);
    void MassSubFromSecond(float m);
    void MassSetFirst(float m);
    void MassSetSecond(float m);

    float MassFirst() { return m_firstM; }
    float MassSecond() { return m_secondM; }
    void MassUnsplitFromFirstToSecond(float m);
};

using FRACTURE_STORAGE = xr_vector<CPHFracture>;
using FRACTURE_I = FRACTURE_STORAGE::iterator;
using FRACTURE_RI = FRACTURE_STORAGE::reverse_iterator;
using element_fracture = std::pair<CPHElement*, CShellSplitInfo>;
using ELEMENT_PAIR_VECTOR = xr_vector<element_fracture>;

class CPHFracturesHolder // stored in CPHElement
{
    friend class CPHElement;
    friend class CPHShellSplitterHolder;
    bool m_has_breaks;

    FRACTURE_STORAGE m_fractures;
    PH_IMPACT_STORAGE m_impacts;
    CFEEDBACK_STORAGE m_feedbacks;
public:
    CPHFracturesHolder();
    ~CPHFracturesHolder();

    void DistributeAdditionalMass(u16 geom_num, float m);
    void SubFractureMass(u16 fracture_num);
    void AddImpact(const Fvector& force, const Fvector& point, u16 id);
    PH_IMPACT_STORAGE& Impacts() { return m_impacts; }
    CPHFracture& LastFracture() { return m_fractures.back(); }
protected:
private:
    u16 CheckFractured();

    element_fracture SplitFromEnd(CPHElement* element, u16 geom_num);
    void PassEndFractures(u16 from, CPHElement* dest);

public:
    void SplitProcess(CPHElement* element, ELEMENT_PAIR_VECTOR& new_elements);
    u16 AddFracture(const CPHFracture& fracture);
    CPHFracture& Fracture(u16 num);

    void PhTune(CharacterVirtualHandle body);
    bool PhDataUpdate(CPHElement* element);
};

IC void sub_diapasones(u16& from1, u16& to1, const u16& from0, const u16& to0)
{
    if (from0 == to0 || from1 == to1 || to1 <= from0 || to1 == u16(-1))
        return;
    R_ASSERT(from0 >= from1 && to0 <= to1);
    u16 dip = to0 - from0;
    to1 = to1 - dip;
}
