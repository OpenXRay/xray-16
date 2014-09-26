//---------------------------------------------------------------------------
#include <vcl.h>
#include <math.h>
#pragma hdrstop
#include "spline.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)


//---------------------------------------------------------------------------
                        Spline::Spline              ()
{
}
//---------------------------------------------------------------------------
                        Spline::~Spline             ()
{
    m_list.clear();
}
//---------------------------------------------------------------------------
void                    Spline::add_point           (float value, float deriv, float time)
{
    for (size_t a = 0; a < m_list.size() - 1; a++)
        if (time > m_list[a].time && time < m_list[a + 1].time)
           {
           point pt = {value, deriv, time};
           m_list.insert (m_list.begin() + a);
           get_point (a, pt);
           return;
           }
}
//---------------------------------------------------------------------------
void                    Spline::remove_point        (float time)
{
    size_t index = find_point (time);
    m_list.erase (m_list.begin() + index);
}
//---------------------------------------------------------------------------
float                   Spline::get_full_time       ()
{
    point_list_i e = m_list.end();
    e--;
    return (*e).time;

}
//---------------------------------------------------------------------------
void                    Spline::reset               ()
{

}
//---------------------------------------------------------------------------
void                    Spline::clear               ()
{
    m_list.clear ();
}
//---------------------------------------------------------------------------
size_t                  Spline::find_point          (float time)
{
    float diff = fabs (m_list[0].time - time);
    size_t index = 0;
    for (size_t a = 0; a < m_list.size (); a++)
        {
        float cdiff = fabs (m_list[a].time - time);
        if (cdiff < diff)
           {
           diff = cdiff;
           index = a;
           }
        }
    return index;
}
//---------------------------------------------------------------------------
float                   Spline::get_value           (float time)
{
    return m_list[find_point (time)].value;
}
//---------------------------------------------------------------------------
float                   Spline::get_deriv           (float time)
{
    return m_list[find_point (time)].deriv;
}
//---------------------------------------------------------------------------
void                    Spline::set_value           (float time, float value)
{
    m_list[find_point (time)].value = value;
}
//---------------------------------------------------------------------------
void                    Spline::set_deriv           (float time, float deriv)
{
    m_list[find_point (time)].deriv = deriv;
}
//---------------------------------------------------------------------------
void                    Spline::create_new          (float time)
{
    clear ();
    point pt = {0.0f, 0.0f, 0.0f};
    m_list.push_back (pt);
    pt.time = time;
    m_list.push_back (pt);
}
//---------------------------------------------------------------------------
float                   Spline::calculate_value     (float time)
{
    //отсеем варианты с корявым временем
    if (time > get_full_time ()) return m_list[m_list.size() - 1].value;
    if (time < 0.0f) return ((*(m_list.begin())).value);

    size_t index;
    for (index = 0; index < m_list.size () - 1; index++)
        if (time >= m_list[index].time && time <= m_list[index + 1].time)
           break;

    float t = (time - m_list[index].time) / (m_list[index + 1].time - m_list[index].time);
    float t2 = t * t;
    float t3 = t * t2;
    return m_list[index].value * (2 * t3 - 3 * t2 + 1) +
           m_list[index].deriv * (t3 - 2 * t2 + 0) +
           m_list[index + 1].value * (-2 * t3 + 3 * t2) +
           m_list[index + 1].deriv * (t3 - t2);

//    f(t) = p1 * (2*t^3 - 3*t^2 + 1) +
//           r1 * (t^3 - 2*t^2 + t) +
//           p2 * (-2*t^3 + 3*t^2) +
//           r2 * (t^3 - t^2).

}
//---------------------------------------------------------------------------
void                    Spline::get_point           (size_t index, point &pt)
{
    if (index >= m_list.size()) index = m_list.size() - 1;
    pt = m_list[index]; 
}

