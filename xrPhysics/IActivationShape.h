#pragma once
class IPhysicsShellHolder;
XRPHYSICS_API void ActivateShapeExplosive				( IPhysicsShellHolder* self_obj, const Fvector &size, Fvector &out_size, Fvector &in_out_pos);
XRPHYSICS_API void ActivateShapePhysShellHolder			(IPhysicsShellHolder *obj, const Fmatrix &in_xform, const Fvector &in_size, Fvector &in_pos, Fvector &out_pos );
XRPHYSICS_API bool ActivateShapeCharacterPhysicsSupport	( Fvector &out_pos, const Fvector &vbox,const Fvector &activation_pos,const Fmatrix &mXFORM, bool not_collide_characters, bool set_rotation, IPhysicsShellHolder *m_EntityAlife );