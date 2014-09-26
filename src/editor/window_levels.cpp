#include "pch.hpp"
#include "window_levels.h"
#include "window_ide.h"
#include "window_view.h"

using editor::window_levels;

Void window_levels::window_levels_Leave		(System::Object^  sender, System::EventArgs^  e)
{
	m_ide->view().property_grid		(PropertyGrid);
}