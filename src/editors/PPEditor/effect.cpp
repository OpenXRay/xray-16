//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop
#include "effect.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)


//---------------------------------------------------------------------------
                    Effect::Effect              (Graphics::TBitmap *host)
{
    m_Host = host;
    m_BaseColor = new ColorParam (host);
    m_AddColor = new ColorParam (host);
    m_GrayColor = new ColorParam (host);
    m_Created = false;
    m_Edit = ee_none;
    m_Time = 0.0f;
}
//---------------------------------------------------------------------------
                    Effect::~Effect             ()
{
    delete m_GrayColor;
    delete m_AddColor;
    delete m_BaseColor;
}
//---------------------------------------------------------------------------
void                Effect::clear               ()
{
    m_BaseColor->clear ();
    m_AddColor->clear ();
    m_GrayColor->clear ();
}
//---------------------------------------------------------------------------
void                Effect::create              (float time)
{
    m_BaseColor->create(time);
    m_AddColor->create(time);
    m_GrayColor->create(time);
    m_Time = time;
    m_Created = true;
}
//---------------------------------------------------------------------------
void                Effect::draw                ()
{
    if (m_Created == false) return;
    m_BaseColor->draw ();
    m_AddColor->draw ();
    m_GrayColor->draw ();
}
//---------------------------------------------------------------------------
void                Effect::add_point           (int _xpos, int _ypos)
{
    if (m_Edit == ee_none) return;
    float ftime = (float)_xpos / (float)m_Host->Width;
    float fvalue = (float)_ypos - (float)m_Host->Height * 0.5f;
    switch (m_Edit)
           {
           case ee_base_color:
                break;
           case ee_add_color:
                break;
           case ee_gray_color:
                break;
           }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
