#ifndef STATE_PREDICATES_INCLUDED
#define STATE_PREDICATES_INCLUDED

namespace award_system
{
// float functions
template <typename T>
class binary_function
{
public:
    virtual bool exec(T const left, T const right) = 0;
}; // float_binary_function

template <typename T>
class ge_function : public binary_function<T>
{
public:
    virtual bool exec(T const left, T const right) { return left >= right; }
}; // class float_ge_function

template <typename T>
class le_function : public binary_function<T>
{
public:
    virtual bool exec(T const left, T const right) { return left <= right; }
}; // float_le_function

template <typename T>
class functions_cf
{
public:
    typedef binary_function<T> function_type;
    enum enum_type_tags
    {
        tt_greater_equal = 0x00,
        tt_less_equal,
        tt_count
    }; // enum type_tags

    static function_type* get_function(enum_type_tags ftype)
    {
        switch (ftype)
        {
        case tt_greater_equal: { return &s_ge_function;
        }
        break;
        case tt_less_equal: { return &s_le_function;
        }
        break;
        }; // switch (ftype)
        return NULL;
    }

private:
    static ge_function<T> s_ge_function;
    static le_function<T> s_le_function;
};

typedef binary_function<float> float_binary_function;
typedef functions_cf<float> float_bfunc_cf;

typedef binary_function<u32> u32_binary_function;
typedef functions_cf<u32> u32_bfunc_cf;

} // namespace award_system

#endif //#ifndef STATE_PREDICATES_INCLUDED
