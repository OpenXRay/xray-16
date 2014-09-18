////////////////////////////////////////////////////////////////////////////
//	Module 		: line_editor.h
//	Created 	: 22.02.2008
//	Author		: Evgeniy Sokolov
//	Description : line editor class, controller of line_edit_control
////////////////////////////////////////////////////////////////////////////

#ifndef LINE_EDITOR_H_INCLUDED
#define LINE_EDITOR_H_INCLUDED

#include "IInputReceiver.h"
#include "line_edit_control.h"

namespace text_editor
{

class line_editor :	public IInputReceiver
{
public:
					line_editor		( u32 str_buffer_size );
	virtual			~line_editor	();

	IC line_edit_control&	control	()				{ return m_control; }

			void	on_frame		();

protected:
	virtual void	IR_OnKeyboardPress	( int dik );
	virtual void	IR_OnKeyboardHold	( int dik );
	virtual void	IR_OnKeyboardRelease( int dik );

private:
	line_edit_control		m_control;

}; // class line_editor

} // namespace text_editor

#endif // LINE_EDITOR_H_INCLUDED
