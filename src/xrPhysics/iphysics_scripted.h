#pragma once

#include "xrServerEntities/smart_cast.h" // get_script_wrapper() needs it
class iphysics_scripted;
class iphysics_game_scripted
{
public:
    virtual ~iphysics_game_scripted(){};
    virtual iphysics_scripted& iphysics_impl() = 0;
    // protected:
    //	virtual						~iphysics_game_scripted ()	=0 {}
};

class iphysics_scripted
{
public:
    virtual void set(iphysics_game_scripted* g) = 0;
    virtual iphysics_game_scripted* get() = 0;
    virtual ~iphysics_scripted(){};
};

class iphysics_scripted_class
{
public:
    // virtual	~iphysics_scripted_class		()	= 0;
    virtual iphysics_scripted& get_scripted() = 0;

protected:
#ifdef _EDITOR
    virtual ~iphysics_scripted_class() {}
#else
#if defined(WINDOWS)
    virtual ~iphysics_scripted_class() = 0 {}
#elif defined(LINUX)
    virtual ~iphysics_scripted_class() {}
#endif
#endif
};

namespace non_copy
{
class noncopyable
{
protected:
    noncopyable() {}
    ~noncopyable() {}
private: // emphasize the following members are private
    noncopyable(const noncopyable&);
    const noncopyable& operator=(const noncopyable&);
};
};

template <class T>
class cphysics_game_scripted : public iphysics_game_scripted, private non_copy::noncopyable
{
    T& impl;

public:
    cphysics_game_scripted(T* im) : impl(*im) {}
    virtual ~cphysics_game_scripted(){};
    virtual iphysics_scripted& iphysics_impl() { return impl.get_scripted(); }
protected:
    virtual T& physics_impl() { return impl; }
    virtual const T& physics_impl() const { return impl; }
public:
    typedef T type_impl;
};

template <class wrap>
wrap* get_script_wrapper(typename wrap::type_impl& E)
{
    wrap* e = smart_cast<wrap*>(E.get_scripted().get());
    if (e)
        return e;

    e = new wrap(&E);
    E.get_scripted().set(e);

    VERIFY(smart_cast<wrap*>(E.get_scripted().get()) == e);

    return e;
}
