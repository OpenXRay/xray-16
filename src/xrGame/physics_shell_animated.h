#pragma once
class	CPhysicsShell;
class	CPhysicsShellHolder;
class physics_shell_animated
{
protected:
	CPhysicsShell			*physics_shell;
	bool					update_velocity;
public:
						physics_shell_animated	( CPhysicsShellHolder* ca, bool	_update_velocity );
	virtual				~physics_shell_animated	( );
	
const	CPhysicsShell	*shell					()const	{ return physics_shell; }
		CPhysicsShell	*shell					()		{ return physics_shell; }
public:
			bool update					( const Fmatrix	&xrorm );
protected:
	virtual	void create_shell			( CPhysicsShellHolder* O );
};
