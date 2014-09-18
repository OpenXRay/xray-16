// new(0)
template <class T>
IC	T*		xr_new		()
{
	return new T();
}
// new(1)
template <class T, class P1>
IC	T*		xr_new		(P1 p1) {
	return new T(p1);
}
// new(2)
template <class T, class P1, class P2> 
IC	T*		xr_new		(P1 p1, P2 p2) {
	return new T(p1,p2);
}
// new(3)
template <class T, class P1, class P2, class P3>
IC	T*		xr_new		(P1 p1, P2 p2, P3 p3) {
	return new T(p1,p2,p3);
}
// new(4)
template <class T, class P1, class P2, class P3, class P4>
IC	T*		xr_new		(P1 p1, P2 p2, P3 p3, P4 p4) {
	return new T(p1,p2,p3,p4);
}
// new(5)
template <class T, class P1, class P2, class P3, class P4, class P5>
IC	T*		xr_new		(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5) {
	return new T(p1,p2,p3,p4,p5);
}
// new(6)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6>
IC	T*		xr_new		(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6) {
	return new T(p1,p2,p3,p4,p5,p6);
}
// new(7)
template <class T, class P1, class P2, class P3, class P4, class P5, class P6, class P7>
IC	T*		xr_new		(P1 p1, P2 p2, P3 p3, P4 p4, P5 p5, P6 p6, P7 p7) {
	return new T(p1,p2,p3,p4,p5,p6,p7);
}

//	#define xr_delete(ptr) if (ptr){delete ptr; ptr=0;}
template <class T>
IC	void	xr_delete	(T* &ptr)
{
	if (ptr) 
	{
		delete ptr;
		const_cast<T*&>(ptr) = NULL;
	}
}
