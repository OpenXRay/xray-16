#pragma once
class IPhysicsShellHolder;
class CPHCharacter;
class IClimableObject
{
public:
    // virtual	const Fvector&	Axis				()const		=0;
    virtual float DDAxis(Fvector& dir) const = 0;
    //
    // virtual	const Fvector&	Side				()const=0;
    virtual float DDSide(Fvector& dir) const = 0;
    //
    virtual const Fvector& Norm() const = 0;
    virtual float DDNorm(Fvector& dir) const = 0;
    virtual bool BeforeLadder(CPHCharacter* actor, float tolerance = 0.f) const = 0;
    virtual float DDLowerP(CPHCharacter* actor, Fvector& out_dir) const = 0; // returns distance and dir to lover point
    virtual float DDUpperP(CPHCharacter* actor, Fvector& out_dir) const = 0; // returns distance and dir to upper point
    //
    // virtual	void			DToAxis				(CPHCharacter	*actor,Fvector &dir)const=0;
    virtual float DDToAxis(CPHCharacter* actor, Fvector& out_dir) const = 0; // returns distance and dir to ladder axis
    // virtual	void			POnAxis				(CPHCharacter	*actor,Fvector	&P)const=0;
    //
    virtual float AxDistToUpperP(CPHCharacter* actor) const = 0;
    virtual float AxDistToLowerP(CPHCharacter* actor) const = 0;
    //
    // virtual	void			DSideToAxis			(CPHCharacter	*actor,Fvector	&dir)const=0;
    // virtual	float			DDSideToAxis		(CPHCharacter	*actor,Fvector	&dir)const=0;
    //
    virtual void DToPlain(CPHCharacter* actor, Fvector& dist) const = 0;
    virtual float DDToPlain(CPHCharacter* actor, Fvector& dir) const = 0;
    // virtual	bool			InRange				(CPHCharacter	*actor)const=0;
    virtual bool InTouch(CPHCharacter* actor) const = 0;
    virtual u16 Material() const = 0;
    //
    // virtual	void			LowerPoint			(Fvector	&P)const=0;
    // virtual	void			UpperPoint			(Fvector	&P)const=0;
    // virtual	void			DefineClimbState	(CPHCharacter	*actor)const=0;

    virtual IPhysicsShellHolder* cast_IPhysicsShellHolder() = 0;

protected:
#if defined(WINDOWS)
    virtual ~IClimableObject() = 0 {}
#elif defined(LINUX)
    virtual ~IClimableObject() = 0;
#endif
};
