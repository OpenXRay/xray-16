#pragma once
#ifndef QUEUED_ASYNC_METHOD_FACADE_H_INCLUDED
#define QUEUED_ASYNC_METHOD_FACADE_H_INCLUDED

template <typename Class, typename ParametersTuple, typename DelegateType,
    void (Class::*method)(ParametersTuple const&, DelegateType),
    void (Class::*release_method)(typename DelegateType::param1_type, typename DelegateType::param2_type)>
class queued_async_method
{
public:
    // delegate parameters
    typedef typename DelegateType::return_type return_type;
    typedef typename DelegateType::param1_type param1_type;
    typedef typename DelegateType::param2_type param2_type;

    queued_async_method() { pending_proxy_exec.bind(this, &queued_async_method::proxy_execution); }
    ~queued_async_method(){};

    void execute(Class* obj_ptr, ParametersTuple const& args, DelegateType func)
    {
        if (current_delegate)
        {
            /*if ((current_obj == obj_ptr) &&
                (current_args == args) &&
                (current_delegate == func))
            {
                return;
            }*/
            pending_obj = obj_ptr;
            pending_args = args;
            pending_delegate = func;
            pending_active = true;
            return;
        }
        pending_active = false;
        current_obj = obj_ptr;
        current_args = args;
        current_delegate = func;

        (current_obj->*method)(current_args, pending_proxy_exec);
    }
    bool is_active() const { return current_delegate; }
    void reexecute() { (current_obj->*method)(current_args, pending_proxy_exec); }
    void stop()
    {
        VERIFY(current_delegate);
        if (current_delegate)
        {
            pending_obj = NULL;
            pending_active = true;
        }
    }

private:
    return_type __stdcall proxy_execution(param1_type arg1, param2_type arg2)
    {
        if (pending_active)
        {
            current_delegate.clear();
            (current_obj->*release_method)(arg1, arg2);
            if (pending_obj)
                execute(pending_obj, pending_args, pending_delegate);
            return;
        }
        current_delegate(arg1, arg2);
        current_delegate.clear();
    }

    bool pending_active;

    Class* pending_obj;
    ParametersTuple pending_args;
    DelegateType pending_delegate;

    Class* current_obj;
    ParametersTuple current_args;
    DelegateType current_delegate;

    DelegateType pending_proxy_exec;
}; // class queued_async_method

struct parameters_tuple0
{
    parameters_tuple0() {}
    parameters_tuple0(parameters_tuple0 const& copy){};

    parameters_tuple0& operator=(parameters_tuple0 const& copy) { return *this; }
    bool operator==(parameters_tuple0 const& right) const { return true; }
}; // class parameters_tuple0

template <typename T1>
struct parameters_tuple1
{
    parameters_tuple1() {}
    parameters_tuple1(T1 t1) : m_t1(t1){};
    parameters_tuple1(parameters_tuple1 const& copy) : m_t1(copy.m_t1){};
    parameters_tuple1& operator=(parameters_tuple1 const& copy)
    {
        m_t1 = copy.m_t1;
        return *this;
    }

    bool operator==(parameters_tuple1 const& right) const { return m_t1 == right.m_t1; }
    T1 m_t1;
}; // class parameters_tuple1

template <typename T1, typename T2>
struct parameters_tuple2
{
    parameters_tuple2() {}
    parameters_tuple2(T1 t1, T2 t2) : m_t1(t1), m_t2(t2){};
    parameters_tuple2(parameters_tuple2 const& copy) : m_t1(copy.m_t1), m_t2(copy.m_t2){};

    parameters_tuple2& operator=(parameters_tuple2 const& copy)
    {
        m_t1 = copy.m_t1;
        m_t2 = copy.m_t2;
        return *this;
    }

    bool operator==(parameters_tuple2 const& right) const { return (m_t1 == right.m_t1) && (m_t2 == right.m_t2); }
    T1 m_t1;
    T2 m_t2;
}; // class parameters_tuple2

template <typename T1, typename T2, typename T3>
struct parameters_tuple3
{
    parameters_tuple3() {}
    parameters_tuple3(T1 t1, T2 t2, T3 t3) : m_t1(t1), m_t2(t2), m_t3(t3){};
    parameters_tuple3(parameters_tuple3 const& copy) : m_t1(copy.m_t1), m_t2(copy.m_t2), m_t3(copy.m_t3){};

    parameters_tuple3& operator=(parameters_tuple3 const& copy)
    {
        m_t1 = copy.m_t1;
        m_t2 = copy.m_t2;
        m_t3 = copy.m_t3;
        return *this;
    }

    bool operator==(parameters_tuple3 const& right) const
    {
        return (m_t1 == right.m_t1) && (m_t2 == right.m_t2) && (m_t3 == right.m_t3);
    }

    T1 m_t1;
    T2 m_t2;
    T3 m_t3;
}; // class parameters_tuple3

template <typename T1, typename T2, typename T3, typename T4>
struct parameters_tuple4
{
    parameters_tuple4() {}
    parameters_tuple4(T1 t1, T2 t2, T3 t3, T4 t4) : m_t1(t1), m_t2(t2), m_t3(t3), m_t4(t4){};
    parameters_tuple4(parameters_tuple4 const& copy)
        : m_t1(copy.m_t1), m_t2(copy.m_t2), m_t3(copy.m_t3), m_t4(copy.m_t4){};

    parameters_tuple4& operator=(parameters_tuple4 const& copy)
    {
        m_t1 = copy.m_t1;
        m_t2 = copy.m_t2;
        m_t3 = copy.m_t3;
        m_t4 = copy.m_t4;
        return *this;
    }

    bool operator==(parameters_tuple4 const& right) const
    {
        return (m_t1 == right.m_t1) && (m_t2 == right.m_t2) && (m_t3 == right.m_t3) && (m_t4 == right.m_t4);
    }

    T1 m_t1;
    T2 m_t2;
    T3 m_t3;
    T4 m_t4;
}; // class parameters_tuple4

#endif //#ifndef QUEUED_ASYNC_METHOD_FACADE_H_INCLUDED
