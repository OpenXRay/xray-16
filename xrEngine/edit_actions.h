////////////////////////////////////////////////////////////////////////////
//	Module 		: edit_actions.h
//	Created 	: 04.03.2008
//	Author		: Evgeniy Sokolov
//	Description : edit actions class
////////////////////////////////////////////////////////////////////////////

#ifndef EDIT_ACTIONS_H_INCLUDED
#define EDIT_ACTIONS_H_INCLUDED

#include <boost/noncopyable.hpp>

namespace text_editor
{

enum	key_state;
class	line_edit_control;

class base : private boost::noncopyable
{
public:
					base			();
	virtual			~base			();
			void	on_assign 		( base* const prev_action );
	virtual	void	on_key_press	( line_edit_control* const control );

protected:
	base*			m_previous_action;

}; // class base

// -------------------------------------------------------------------------------------------------

class callback_base : public base
{
private:
	typedef		fastdelegate::FastDelegate0<void>		Callback;

public:
					callback_base	( Callback const& callback, key_state state );
	virtual			~callback_base	();
	virtual	void	on_key_press	( line_edit_control* const control );

protected:
	key_state	m_run_state;
	Callback	m_callback;

}; // class callback_base

// -------------------------------------------------------------------------------------------------

class type_pair : public base
{
public:
					type_pair		( u32 dik, char c, char c_shift, bool b_translate );
	virtual			~type_pair		();
			void	init			( u32 dik, char c, char c_shift, bool b_translate );
	virtual	void	on_key_press	( line_edit_control* const control );

private:
	u32		m_dik;
	bool	m_translate;
	char	m_char;
	char	m_char_shift;

}; // class type_pair

// -------------------------------------------------------------------------------------------------

class key_state_base : public base
{
public:
					key_state_base	( key_state state, base* type_pair );
	virtual			~key_state_base	();
	virtual	void	on_key_press	( line_edit_control* const control );

private:
	key_state		m_state;
	base*			m_type_pair;
}; // class key_state_base

} // namespace text_editor

#endif // EDIT_ACTIONS_H_INCLUDED
