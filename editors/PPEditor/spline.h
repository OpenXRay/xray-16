//---------------------------------------------------------------------------
#ifndef splineH
#define splineH
//---------------------------------------------------------------------------
#include <vector>
#include <list>

using namespace std;

typedef struct _point
{
    float   value;              //значение функции
    float   deriv;              //производная
    float   time;               //время
} point;

typedef vector<point>           point_list;
typedef vector<point>::iterator point_list_i;

class Spline
{
private:
protected:
    point_list              m_list;
    size_t                  find_point          (float time);
public:
                            Spline              ();
                           ~Spline              ();
    void                    add_point           (float value, float deriv, float time);
    void                    remove_point        (float time);
    float                   get_full_time       ();
    void                    reset               ();
    void                    clear               ();
    float                   get_value           (float time);
    float                   get_deriv           (float time);
    void                    set_value           (float time, float value);
    void                    set_deriv           (float time, float deriv);
    void                    create_new          (float time);
    float                   calculate_value     (float time);
    size_t                  get_points_count    () { return m_list.size(); }
    void                    get_point           (size_t index, point &pt);
};
#endif
