////////////////////////////////////////////////////////////////////////////
//	Module 		: intrusive_ptr.h
//	Created 	: 30.07.2004
//  Modified 	: 30.07.2004
//	Author		: Dmitriy Iassenev
//	Description : Intrusive pointer template
////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef XRGAME_EXPORTS
#	include "object_type_traits.h"
#endif //XRGAME_EXPORTS

#pragma pack(push,4)

struct intrusive_base {
	u32		m_ref_count;

	IC			intrusive_base	() : m_ref_count(0)
	{
	}

	template <typename T>
	IC	void	_release		(T*object)
	{
		try {
			xr_delete	(object);
		}
		catch(...) {
		}
	}
};

template <typename object_type, typename base_type = intrusive_base>
class intrusive_ptr {
private:
	typedef base_type								base_type;
	typedef object_type								object_type;
	typedef intrusive_ptr<object_type,base_type>	self_type;
	typedef const object_type *(intrusive_ptr::*unspecified_bool_type) () const;

private:
#ifdef XRGAME_EXPORTS
	enum {result = 
		object_type_traits::is_base_and_derived<base_type,object_type>::value ||
		object_type_traits::is_same<base_type,object_type>::value
	};
#else 
	enum {result = true	};
#endif
private:
	object_type					*m_object;

protected:
	IC		void				dec				();

public:
	IC							intrusive_ptr	();
	IC							intrusive_ptr	(object_type *rhs);
	IC							intrusive_ptr	(self_type const &rhs);
	IC							~intrusive_ptr	();
	IC		self_type			&operator=		(object_type* rhs);
	IC		self_type			&operator=		(self_type const &rhs);
	IC		object_type			&operator*		() const;
	IC		object_type			*operator->		() const;
	IC		bool				operator!		() const;
	IC							operator unspecified_bool_type () const {	return(!m_object? 0: &intrusive_ptr::get);}
	IC		u32					size			();
	IC		void				swap			(self_type &rhs);
	IC		bool				equal			(const self_type & rhs)	const;
	IC		void				set				(object_type* rhs);
	IC		void				set				(self_type const &rhs);
	IC		const object_type	*get			()	const;
};

template <typename object_type, typename base_type>
IC			bool				operator==		(intrusive_ptr<object_type,base_type> const & a, intrusive_ptr<object_type,base_type> const & b);

template <typename object_type, typename base_type>
IC			bool				operator!=		(intrusive_ptr<object_type,base_type> const & a, intrusive_ptr<object_type,base_type> const & b);

template <typename object_type, typename base_type>
IC			bool				operator<		(intrusive_ptr<object_type,base_type> const & a, intrusive_ptr<object_type,base_type> const & b);

template <typename object_type, typename base_type>
IC			bool				operator>		(intrusive_ptr<object_type,base_type> const & a, intrusive_ptr<object_type,base_type> const & b);

template <typename object_type, typename base_type>
IC			void				swap			(intrusive_ptr<object_type,base_type> & lhs, intrusive_ptr<object_type,base_type> & rhs);


#include "intrusive_ptr_inline.h"

#pragma pack(pop)