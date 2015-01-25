// luabind::auto_ptr implementation is copied from the stlport std::auto_ptr (v5.1)

#ifndef LUABIND_AUTO_PTR_H_INCLUDED
#define LUABIND_AUTO_PTR_H_INCLUDED

namespace luabind {

class __ptr_base {
public:
  void* _M_p;
  void  __set(const void* p) { _M_p = const_cast<void*>(p); }
  void  __set(void* p) { _M_p = p; }
}; // class __ptr_base

template <class _Tp>
class auto_ptr_ref {
public:
	__ptr_base& _M_r;
	_Tp* const _M_p;

	auto_ptr_ref(__ptr_base& __r, _Tp* __p) : _M_r(__r), _M_p(__p) {  }

	_Tp* release() const { _M_r.__set((void*)0); return _M_p; }

private:
	//explicitely defined as private to avoid warnings:
	typedef auto_ptr_ref<_Tp> _Self;
	_Self& operator = (_Self const&);
}; // class auto_ptr_ref

template<class _Tp>
class auto_ptr : public __ptr_base {
public:
	typedef _Tp				element_type;
	typedef auto_ptr<_Tp>	_Self;

public:
	_Tp* release	()
	{
		_Tp* __px	= this->get();
		this->_M_p	= 0;
		return		__px;
	}

	void reset		(_Tp* __px = 0)
	{
		_Tp* __pt	= this->get();
		if (__px != __pt)
			luabind_delete	(__pt);
		this->__set	(__px);
	}

	_Tp* get		() const
	{
		return		reinterpret_cast<_Tp*>(const_cast<void*>(_M_p));
	}

	_Tp* operator->	() const
	{
		VERIFY2		(get(), "auto_ptr is null");
		return		get();
	}

	_Tp& operator*	() const
	{
		VERIFY2		(get(), "auto_ptr is null");
		return		*get();
	}

	explicit auto_ptr(_Tp* __px = 0)
	{
		this->__set	(__px);
	}

	template<class _Tp1> auto_ptr(auto_ptr<_Tp1>& __r)
	{
		_Tp* __conversionCheck = __r.release();
		this->__set	(__conversionCheck);
	}

	template<class _Tp1> auto_ptr<_Tp>& operator=(auto_ptr<_Tp1>& __r)
	{
		_Tp* __conversionCheck = __r.release();
		reset		(__conversionCheck);
		return		*this;
	}

	auto_ptr		(_Self& __r)
	{
		this->__set	(__r.release());
	}

	_Self& operator=(_Self& __r)
	{
		reset		(__r.release());
		return		(*this);
	}

	~auto_ptr		()
	{
		_Tp* __pt		= this->get();
		luabind_delete	(__pt);
	}

	auto_ptr		(auto_ptr_ref<_Tp> __r)
	{
		this->__set(__r.release());
	}

	_Self& operator=(auto_ptr_ref<_Tp> __r)
	{
		reset		(__r.release());
		return		*this;
	}

	template<class _Tp1> operator auto_ptr_ref<_Tp1>()
	{
		return		auto_ptr_ref<_Tp1>(*this, this->get());
	}

	template<class _Tp1> operator auto_ptr<_Tp1>()
	{
		return		(auto_ptr<_Tp1>(release()));
	}
}; // class auto_ptr

} // namespace luabind
	
#endif // #ifndef LUABIND_AUTO_PTR_H_INCLUDED