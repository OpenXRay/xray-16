#ifndef PHYSICS_COMMON_H
#define PHYSICS_COMMON_H

#include "DisablingParams.h"
#include "PhysicsExternalCommon.h"

extern XRPHYSICS_API const float default_l_limit;
extern XRPHYSICS_API const float default_w_limit;
extern XRPHYSICS_API const float default_k_l;
extern XRPHYSICS_API const float default_k_w;
extern const float default_l_scale;
extern const float default_w_scale;

extern const float base_fixed_step;
extern const float base_erp;
extern const float base_cfm;

extern XRPHYSICS_API float fixed_step;
extern float world_cfm;
extern float world_erp;
extern float world_spring;
extern float world_damping;

// extern const float	mass_limit										;
extern const u16 max_joint_allowed_for_exeact_integration;
extern XRPHYSICS_API const float default_world_gravity;
extern XRPHYSICS_API float phTimefactor;
extern XRPHYSICS_API int phIterations;
// extern	float		phBreakCommonFactor								;
// extern	float		phRigidBreakWeaponFactor						;
// extern	float		ph_tri_query_ex_aabb_rate						;
// extern	int			ph_tri_clear_disable_count						;

struct SGameMtl;
#define ERP_S(k_p, k_d, s) ((s * (k_p)) / (((s) * (k_p)) + (k_d)))
#define CFM_S(k_p, k_d, s) (1.f / (((s) * (k_p)) + (k_d)))
#define SPRING_S(cfm, erp, s) ((erp) / (cfm) / s)

//////////////////////////////////////////////////////////////////////////////////
#define DAMPING(cfm, erp) ((1.f - (erp)) / (cfm))
#define ERP(k_p, k_d) ERP_S(k_p, k_d, fixed_step)
#define CFM(k_p, k_d) CFM_S(k_p, k_d, fixed_step)
#define SPRING(cfm, erp) SPRING_S(cfm, erp, fixed_step)

IC float Erp(float k_p, float k_d, float s = fixed_step) { return ((s * (k_p)) / (((s) * (k_p)) + (k_d))); }
IC float Cfm(float k_p, float k_d, float s = fixed_step) { return (1.f / (((s) * (k_p)) + (k_d))); }
IC float Spring(float cfm, float erp, float s = fixed_step) { return ((erp) / (cfm) / s); }
IC float Damping(float cfm, float erp) { return ((1.f - (erp)) / (cfm)); }
IC void MulSprDmp(float& cfm, float& erp, float mul_spring, float mul_damping)
{
    float factor = 1.f / (mul_spring * erp + mul_damping * (1 - erp));
    cfm *= factor;
    erp *= (factor * mul_spring);
}

#endif // PHYSICS_COMMON_H
