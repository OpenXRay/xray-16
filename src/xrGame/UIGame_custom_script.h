#pragma once
#include "UIGameCustom.h"
#include "script_export_space.h"
#include "game_cl_Base.h"

class UIGame_custom_script : public CUIGameCustom
{
	typedef CUIGameCustom inherited;
public:
									UIGame_custom_script		():inherited(){};
	virtual void					SetClGame					(game_cl_GameState* g){inherited::SetClGame(g);};
	virtual	void					Init						(){};
	DECLARE_SCRIPT_REGISTER_FUNCTION
};
add_to_type_list(UIGame_custom_script)
#undef script_type_list
#define script_type_list save_type_list(UIGame_custom_script)
