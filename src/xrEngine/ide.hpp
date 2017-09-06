////////////////////////////////////////////////////////////////////////////
// Module : ide.hpp
// Created : 11.12.2007
// Modified : 29.12.2007
// Author : Dmitriy Iassenev
// Description : editor ide function
////////////////////////////////////////////////////////////////////////////

#ifndef IDE_HPP_INCLUDED
#define IDE_HPP_INCLUDED

#ifdef INGAME_EDITOR

#include "Include/editor/ide.hpp"

namespace editor
{
class ide_base;
} // namespace editor

inline editor::ide_base& ide()
{
    VERIFY(Device.editor());
    return (*Device.editor());
}

#endif // #ifdef INGAME_EDITOR

#endif // ifndef IDE_HPP_INCLUDED
