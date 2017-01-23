#pragma	once

#include "physics_shell_animated.h"

class	CGameObject;
class	CBlend;
struct	dContact;
struct	SGameMtl;

class interactive_animation:
	public physics_shell_animated
{
typedef	physics_shell_animated inherited;
	CBlend					*blend;
public:
			interactive_animation	( CPhysicsShellHolder* ca, CBlend* b );
virtual		~interactive_animation	( );

public:
virtual		bool update					( const Fmatrix	&xrorm );

private:
virtual		void create_shell			( CPhysicsShellHolder* O );
		bool collide				( );
static	void contact_callback		( bool& do_colide, bool bo1, dContact& c,SGameMtl * /*material_1*/, SGameMtl * /*material_2*/ );
};