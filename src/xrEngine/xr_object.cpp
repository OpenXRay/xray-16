#include "xr_object.h"
#include "Common/LevelStructure.hpp"
#include "GameFont.h"
#include "IGame_Level.h"
#include "Include/xrRender/Kinematics.h"
#include "Include/xrRender/RenderVisual.h"
#include "Render.h"
#include "mp_logging.h"
#include "stdafx.h"
#include "x_ray.h"
#include "xrCDB/xr_area.h"
#include "xr_collide_form.h"

inline void CObjectList::o_crow(IGameObject* O)
{
    Objects& crows = get_crows();
    VERIFY(std::find(crows.begin(), crows.end(), O) == crows.end());
    crows.push_back(O);

    O->SetCrowUpdateFrame(Device.dwFrame);
}
