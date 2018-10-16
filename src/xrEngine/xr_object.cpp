#include "stdafx.h"
#include "IGame_Level.h"
#include "xr_object.h"
#include "xrCDB/xr_area.h"
#include "Render.h"
#include "Common/LevelStructure.hpp"
#include "Include/xrRender/RenderVisual.h"
#include "Include/xrRender/Kinematics.h"
#include "x_ray.h"
#include "GameFont.h"
#include "mp_logging.h"
#include "xr_collide_form.h"

void CObjectList::o_crow(IGameObject* O)
{
    Objects& crows = get_crows();
    VERIFY(std::find(crows.begin(), crows.end(), O) == crows.end());
    crows.push_back(O);

    O->SetCrowUpdateFrame(Device.dwFrame);
}
