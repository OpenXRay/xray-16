#ifndef __RANGE_T_H__
#define __RANGE_T_H__

/*!
        Class _range_t, can represent anything that is range between two values, mostly used for time ranges.
 */
template <class T>
class _range_t
{
public:
    T start;
    T end;

    _range_t()
    {
        start = T(0);
        end = T(0);
    };
    _range_t(const _range_t& r)
    {
        start = r.start;
        end = r.end;
    };
    _range_t(T s, T e)
    {
        start = s;
        end = e;
    };

    void Set(T s, T e)
    {
        start = s;
        end = e;
    };
    void Clear()
    {
        start = 0;
        end = 0;
    };

    //! Get length of range.
    T Length() const { return end - start; };
    //! Check if range is empty.
    bool IsEmpty() const { return (start == 0 && end == 0); }

    //! Check if value is inside range.
    bool IsInside(T val) { return val >= start && val <= end; };

    void ClipValue(T& val) const
    {
        if (val < start)
        {
            val = start;
        }
        if (val > end)
        {
            val = end;
        }
    }

    //! Compare two ranges.
    bool operator==(const _range_t& r) const { return start == r.start && end == r.end; }

    bool operator!=(const _range_t& r) const { return !(*this == r); }
    //! Assign operator.
    _range_t& operator=(const _range_t& r)
    {
        start = r.start;
        end = r.end;
        return *this;
    }
    //! Interect two ranges.
    _range_t operator&(const _range_t& r) const { return _range_t(MAX(start, r.start), MIN(end, r.end)); }
    _range_t& operator&=(const _range_t& r) { return (*this = (*this & r)); }
    //! Concatent two ranges.
    _range_t operator|(const _range_t& r) const { return _range_t(MIN(start, r.start), MAX(end, r.end)); }
    _range_t& operator|=(const _range_t& r) { return (*this = (*this | r)); }
    //! Add new value to range.
    _range_t operator+(T v) const
    {
        T s = start, e = end;
        if (v < start)
        {
            s = v;
        }
        if (v > end)
        {
            e = v;
        }
        return _range_t(s, e);
    }
    //! Add new value to range.
    _range_t& operator+=(T v) const
    {
        if (v < start)
        {
            start = v;
        }
        if (v > end)
        {
            end = v;
        }
        return *this;
    }
};

#endif