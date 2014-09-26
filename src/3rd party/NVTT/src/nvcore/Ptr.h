// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_PTR_H
#define NV_CORE_PTR_H

#include <nvcore/nvcore.h>
#include <nvcore/Debug.h>

#include <stdio.h>	// NULL

namespace nv
{
	
/** Simple auto pointer template class.
 *
 * This is very similar to the standard auto_ptr class, but with some 
 * additional limitations to make its use less error prone:
 * - Copy constructor and assignment operator are disabled.
 * - reset method is removed.
 * 
 * The semantics of the standard auto_ptr are not clear and change depending
 * on the std implementation. For a discussion of the problems of auto_ptr read:
 * http://www.awprofessional.com/content/images/020163371X/autoptrupdate\auto_ptr_update.html
 */
template <class T>
class AutoPtr
{
	NV_FORBID_COPY(AutoPtr);
	NV_FORBID_HEAPALLOC();
public:
	
	/// Default ctor.
	AutoPtr() : m_ptr(NULL) { }
	
	/// Ctor.
	explicit AutoPtr( T * p ) : m_ptr(p) { }
	
	/** Dtor. Deletes owned pointer. */
	~AutoPtr() {
		delete m_ptr;
		m_ptr = NULL;
	}

	/** Delete owned pointer and assign new one. */
	void operator=( T * p ) {
		if (p != m_ptr)
		{
			delete m_ptr;
			m_ptr = p;
		}
	}

	/** Member access. */
	T * operator -> () const {
		nvDebugCheck(m_ptr != NULL);
		return m_ptr;
	}

	/** Get reference. */
	T & operator*() const {
		nvDebugCheck(m_ptr != NULL);
		return *m_ptr;
	}

	/** Get pointer. */
	T * ptr() const { return m_ptr; }
	
	/** Relinquish ownership of the underlying pointer and returns that pointer. */
	T * release() {
		T * tmp = m_ptr;
		m_ptr = NULL;
		return tmp;
	}
	
	/** Const pointer equal comparation. */
	friend bool operator == (const AutoPtr<T> & ap, const T * const p) {
		return (ap.ptr() == p);
	}

	/** Const pointer nequal comparation. */
	friend bool operator != (const AutoPtr<T> & ap, const T * const p) {
		return (ap.ptr() != p);
	}

	/** Const pointer equal comparation. */
	friend bool operator == (const T * const p, const AutoPtr<T> & ap) {
		return (ap.ptr() == p);
	}

	/** Const pointer nequal comparation. */
	friend bool operator != (const T * const p, const AutoPtr<T> & ap) {
		return (ap.ptr() != p);
	}

private:
	T * m_ptr;
};

#if 0
/** Reference counted base class to be used with Pointer.
 *
 * The only requirement of the Pointer class is that the RefCounted class implements the 
 * addRef and release methods.
 */
class RefCounted
{
	NV_FORBID_COPY(RefCounted);
public:

	/// Ctor.
	RefCounted() : m_count(0), m_weak_proxy(NULL)
	{
		s_total_obj_count++;
	}

	/// Virtual dtor.
	virtual ~RefCounted()
	{
		nvCheck( m_count == 0 );
		nvCheck( s_total_obj_count > 0 );
		s_total_obj_count--;
	}


	/// Increase reference count.
	uint addRef() const
	{
		s_total_ref_count++;
		m_count++;
		return m_count;
	}


	/// Decrease reference count and remove when 0.
	uint release() const
	{
		nvCheck( m_count > 0 );
		
		s_total_ref_count--;
		m_count--;
		if( m_count == 0 ) {
			releaseWeakProxy();
			delete this;
			return 0;
		}
		return m_count;
	}

	/// Get weak proxy.
	WeakProxy * getWeakProxy() const
	{
		if (m_weak_proxy == NULL) {
			m_weak_proxy = new WeakProxy;
			m_weak_proxy->AddRef();
		}
		return m_weak_proxy;
	}

	/// Release the weak proxy.	
	void releaseWeakProxy() const
	{
		if (m_weak_proxy != NULL) {
			m_weak_proxy->NotifyObjectDied();
			m_weak_proxy->Release();
			m_weak_proxy = NULL;
		}
	}

	/** @name Debug methods: */
	//@{
		/// Get reference count.
		int refCount() const
		{
			return m_count;
		}

		/// Get total number of objects.
		static int totalObjectCount()
		{
			return s_total_obj_count;
		}

		/// Get total number of references.
		static int totalReferenceCount()
		{
			return s_total_ref_count;
		}
	//@}


private:

	NVCORE_API static int s_total_ref_count;
	NVCORE_API static int s_total_obj_count;

	mutable int m_count;
	mutable WeakProxy * weak_proxy;

};
#endif

/// Smart pointer template class.
template <class BaseClass>
class Pointer {
public:

	// BaseClass must implement addRef() and release().
	typedef Pointer<BaseClass>	ThisType;

	/// Default ctor.
	Pointer() : m_ptr(NULL) 
	{
	}

	/** Other type assignment. */
	template <class OtherBase>
	Pointer( const Pointer<OtherBase> & tc )
	{
		m_ptr = static_cast<BaseClass *>( tc.ptr() );
		if( m_ptr ) {
			m_ptr->addRef();
		}
	}

	/** Copy ctor. */
	Pointer( const ThisType & bc )
	{
		m_ptr = bc.ptr();
		if( m_ptr ) {
			m_ptr->addRef();
		}
	}

	/** Copy cast ctor. Pointer(NULL) is valid. */
	explicit Pointer( BaseClass * bc )
	{
		m_ptr = bc;
		if( m_ptr ) {
			m_ptr->addRef();
		}
	}

	/** Dtor. */
	~Pointer()
	{
		set(NULL);
	}


	/** @name Accessors: */
	//@{
		/** -> operator. */
		BaseClass * operator -> () const
		{
			nvCheck( m_ptr != NULL );
			return m_ptr;
		}

		/** * operator. */
		BaseClass & operator*() const
		{
			nvCheck( m_ptr != NULL );
			return *m_ptr;
		}

		/** Get pointer. */
		BaseClass * ptr() const
		{
			return m_ptr;
		}
	//@}


	/** @name Mutators: */
	//@{
		/** Other type assignment. */
		template <class OtherBase>
		void operator = ( const Pointer<OtherBase> & tc )
		{
			set( static_cast<BaseClass *>(tc.ptr()) );
		}

		/** This type assignment. */
		void operator = ( const ThisType & bc )
		{
			set( bc.ptr() );
		}

		/** Pointer assignment. */
		void operator = ( BaseClass * bc )
		{
			set( bc );
		}
	//@}


	/** @name Comparators: */
	//@{
		/** Other type equal comparation. */
		template <class OtherBase>
		bool operator == ( const Pointer<OtherBase> & other ) const
		{
			return m_ptr == other.ptr();
		}

		/** This type equal comparation. */
		bool operator == ( const ThisType & bc ) const
		{
			return m_ptr == bc.ptr();
		}

		/** Const pointer equal comparation. */
		bool operator == ( const BaseClass * const bc ) const
		{
			return m_ptr == bc;
		}

		/** Other type not equal comparation. */
		template <class OtherBase>
		bool operator != ( const Pointer<OtherBase> & other ) const
		{
			return m_ptr != other.ptr();
		}
		
		/** Other type not equal comparation. */
		bool operator != ( const ThisType & bc ) const
		{
			return m_ptr != bc.ptr();
		}

		/** Const pointer not equal comparation. */
		bool operator != (const BaseClass * const bc) const
		{
			return m_ptr != bc;
		}

		/** This type lower than comparation. */
		bool operator < (const ThisType & p) const
		{
			return m_ptr < p.ptr();
		}
	//@}

private:

	/** Set this pointer. */
	void set( BaseClass * p )
	{
		if( m_ptr != p ) {
			if( m_ptr ) m_ptr->release();
			if( p ) p->addRef();
			m_ptr = p;
		}
	}

private:

	BaseClass * m_ptr;

};

} // nv namespace

#endif // NV_CORE_PTR_H
