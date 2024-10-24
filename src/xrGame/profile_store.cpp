#include "StdAfx.h"
#include "profile_store.h"
#include "MainMenu.h"
#include "login_manager.h"

namespace gamespy_profile
{
void profile_store::load_current_profile(store_operation_cb /*progress_indicator_cb*/, store_operation_cb complete_cb)
{
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    if (tmp_lmngr && tmp_lmngr->get_current_profile())
    {
        if (complete_cb)
            complete_cb(true, "");
        return;
    }
    complete_cb(false, "mp_first_need_to_login");
}
} // namespace gamespy_profile
