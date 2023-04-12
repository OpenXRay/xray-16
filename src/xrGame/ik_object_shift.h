#pragma once
namespace extrapolation
{
class points;
}
class object_shift
{
    float current{};
    float taget{};
    float taget_time{};
    float current_time{};
    float speed{};
    float accel{};
    float aaccel{};
    bool b_freeze{};

public:
    object_shift() = default;
    void set_taget(float taget, float time);
    float shift() const;
    void freeze(bool v) { b_freeze = v; }
private:
    float shift(float time_global) const;
    float delta_shift(float delta_time) const;
#ifdef DEBUG
public:
    void dbg_draw(const Fmatrix& current_pos, const extrapolation::points& predict, const Fvector& start) const;

private:
#endif
};
