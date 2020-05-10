#include "stdafx.h"
#include "IGame_Level.h"
#include "Feel_Touch.h"
#include "xr_object.h"
using namespace Feel;

Touch::Touch() : pure_relcase(&Touch::feel_touch_relcase) {}
Touch::~Touch() {}
bool Touch::feel_touch_contact(IGameObject* O) { return true; }
void Touch::feel_touch_deny(IGameObject* O, u32 T)
{
    DenyTouch D;
    D.O = O;
    D.Expire = Device.dwTimeGlobal + T;
    feel_touch_disable.push_back(D);
}

void Touch::feel_touch_update(Fvector& C, float R)
{
    // Check if denied objects expire in time
    u32 dwT = Device.dwTimeGlobal;
    for (u32 dit = 0; dit < feel_touch_disable.size(); dit++)
    {
        if (feel_touch_disable[dit].Expire < dwT)
        {
            feel_touch_disable.erase(feel_touch_disable.begin() + dit);
            dit--;
        }
    }

    // Find nearest objects
    q_nearest.clear();
    q_nearest.reserve(feel_touch.size());
    g_pGameLevel->ObjectSpace.GetNearest(q_nearest, C, R, NULL);
    xr_vector<IGameObject*>::iterator n_begin = q_nearest.begin();
    xr_vector<IGameObject*>::iterator n_end = q_nearest.end();
    if (n_end != n_begin)
    {
        // Process results (NEW)
        for (xr_vector<IGameObject*>::iterator it = n_begin; it != n_end; ++it)
        {
            IGameObject* O = *it;
            if (O->getDestroy())
                continue; // Don't touch candidates for destroy
            if (!feel_touch_contact(O))
                continue; // Actual contact

            if (std::find(feel_touch.begin(), feel_touch.end(), O) == feel_touch.end())
            {
                // check for deny
                bool bDeny = false;
                for (u32 dit = 0; dit < feel_touch_disable.size(); dit++)
                    if (O == feel_touch_disable[dit].O)
                    {
                        bDeny = true;
                        break;
                    }

                // _new _
                if (!bDeny)
                {
                    feel_touch.push_back(O);
                    feel_touch_new(O);
                }
            }
        }
    }

    // Process results (DELETE)
    for (int d = 0; d < int(feel_touch.size()); d++)
    {
        IGameObject* O = feel_touch[d];
        if (O->getDestroy() || !feel_touch_contact(O) ||
            (std::find(n_begin, n_end, O) == n_end)) // Don't touch candidates for destroy
        {
            // _delete_
            feel_touch.erase(feel_touch.begin() + d);
            feel_touch_delete(O);
            d--;
        }
    }

    //. Engine.Sheduler.Slice ();
}

void Touch::feel_touch_relcase(IGameObject* O)
{
    xr_vector<IGameObject*>::iterator I = std::find(feel_touch.begin(), feel_touch.end(), O);
    if (I != feel_touch.end())
    {
        feel_touch.erase(I);
        feel_touch_delete(O);
    }
    xr_vector<DenyTouch>::iterator Id = feel_touch_disable.begin(), IdE = feel_touch_disable.end();
    for (; Id != IdE; ++Id)
        if ((*Id).O == O)
        {
            feel_touch_disable.erase(Id);
            break;
        }
}
