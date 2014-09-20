#pragma once

class IKinematicsAnimated;
class CBlend;
class CGameObject;
class	anim_script_callback
{
	bool	on_end;
	bool	on_begin;
	bool	is_set;
private:
	static	 void	anim_callback		( CBlend* P );
public:
			anim_script_callback	(): on_end( false ), on_begin( false ), is_set( false ){}
	CBlend*	play_cycle				( IKinematicsAnimated* sa, const shared_str& anim );
	void	update					(CGameObject &O);
};