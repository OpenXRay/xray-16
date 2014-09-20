//---------------------------------------------------------------------------

#ifndef effectH
#define effectH
#include "color_param.h"
//---------------------------------------------------------------------------

typedef enum _edit_effect
{
    ee_none             =   0,
    ee_base_color       =   1,
    ee_add_color        =   2,
    ee_gray_color       =   3,
    ee_force_dword      =   0x7fffffff
} edit_effect;


class Effect
{
protected:
    Graphics::TBitmap                      *m_Host;
    ColorParam                             *m_BaseColor;
    ColorParam                             *m_AddColor;
    ColorParam                             *m_GrayColor;
    bool                                    m_Created;
    edit_effect                             m_Edit;
    float                                   m_Time;
public:
                        Effect              (Graphics::TBitmap *host);
                       ~Effect              ();
    void                clear               ();
    void                create              (float time);
    void                draw                ();
    ColorParam*         get_base_color      () { return m_BaseColor; }
    ColorParam*         get_add_color       () { return m_AddColor; }
    ColorParam*         get_gray_color      () { return m_GrayColor; }
    void                add_point           (int _xpos, int _ypos);
};
#endif
