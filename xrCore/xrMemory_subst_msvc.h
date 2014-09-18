#ifdef DEBUG_MEMORY_NAME
// new(0)
template <class T>
IC	T*		xr_new		()
{
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T();
}
// new(1)
template <class T, class P1>
IC	T*		xr_new		(const P1& p1) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1);
}
// new(2)
template <class T, class P1, class P2> 
IC	T*		xr_new		(const P1& p1, const P2& p2) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2);
}
// new(3)
template <class T, class P1, class P2, class P3>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3);
}
// new(4)
template <class T, class P1, class P2, class P3, class P4>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4);
}
// new(5)
template <class T, class P1, class P2, class P3, class P4, class P5>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4,p5);
}
// new(6)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4,p5,p6);
}
// new(7)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7);
}
// new(8)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7,p8);
}
// new(9)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8, const P8& p9) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T), typeid(T).name());
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7,p8,p9);
}
#else // DEBUG_MEMORY_NAME
// new(0)
template <class T>
IC	T*		xr_new		()
{
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T();
}
// new(1)
template <class T, class P1>
IC	T*		xr_new		(const P1& p1) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1);
}
// new(2)
template <class T, class P1, class P2> 
IC	T*		xr_new		(const P1& p1, const P2& p2) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2);
}
// new(3)
template <class T, class P1, class P2, class P3>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3);
}
// new(4)
template <class T, class P1, class P2, class P3, class P4>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4);
}
// new(5)
template <class T, class P1, class P2, class P3, class P4, class P5>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4,p5);
}
// new(6)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4,p5,p6);
}
// new(7)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7);
}
// new(8)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7,p8);
}
// new(9)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7, class P8, class P9>
IC	T*		xr_new		(const P1& p1, const P2& p2, const P3& p3, const P4& p4, const P5& p5, const P6& p6, const P7& p7, const P8& p8, const P8& p9) {
	T* ptr	= (T*)Memory.mem_alloc(sizeof(T));
	return new (ptr) T(p1,p2,p3,p4,p5,p6,p7,p8,p9);
}
#endif // DEBUG_MEMORY_NAME

template <bool _is_pm, typename T>
struct xr_special_free
{
	IC void operator()(T* &ptr)
	{
		void*	_real_ptr	= dynamic_cast<void*>(ptr);
		ptr->~T			();
		Memory.mem_free	(_real_ptr);
	}
};

template <typename T>
struct xr_special_free<false,T>
{
	IC void operator()(T* &ptr)
	{
		ptr->~T			();
		Memory.mem_free	(ptr);
	}
};

template <class T>
IC	void	xr_delete	(T* &ptr)
{
	if (ptr) 
	{
		xr_special_free<is_polymorphic<T>::result,T>()(ptr);
		ptr = NULL;
	}
}
template <class T>
IC	void	xr_delete	(T* const &ptr)
{
	if (ptr) 
	{
		xr_special_free<is_polymorphic<T>::result,T>(ptr);
		const_cast<T*&>(ptr) = NULL;
	}
}

#ifdef DEBUG_MEMORY_MANAGER
	void XRCORE_API mem_alloc_gather_stats				(const bool &value);
	void XRCORE_API mem_alloc_gather_stats_frequency	(const float &value);
	void XRCORE_API mem_alloc_show_stats				();
	void XRCORE_API mem_alloc_clear_stats				();
#endif // DEBUG_MEMORY_MANAGER