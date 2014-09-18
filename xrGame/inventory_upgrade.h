////////////////////////////////////////////////////////////////////////////
//	Module 		: inventory_upgrade.h
//	Created 	: 01.11.2007
//  Modified 	: 27.11.2007
//	Author		: Evgeniy Sokolov
//	Description : inventory upgrade class
////////////////////////////////////////////////////////////////////////////

#ifndef INVENTORY_UPGRADE_H_INCLUDED
#define INVENTORY_UPGRADE_H_INCLUDED

#include "inventory_upgrade_base.h"

namespace inventory
{
namespace upgrade
{

namespace detail
{

template <typename return_type>
struct functor_base
{
	typedef luabind::functor<return_type>	functor_type;

	functor_type	functr;
	LPCSTR			parameter;
};

template <typename return_type>
struct functor : public functor_base<return_type>
{
	IC	return_type	operator()	() const
	{
		return	functr( parameter );
	}
};

template <typename return_type>
struct functor2 : public functor<return_type>
{
	LPCSTR			parameter2;

	IC	return_type	operator()	() const
	{
		return	functr( parameter, parameter2 );
	}
};

template <typename return_type>
struct functor3 : public functor2<return_type>
{
	int				parameter3;

	IC	return_type	operator()	() const
	{
		return	functr( parameter, parameter2, parameter3 );
	}
};

template <>
struct functor<void> : public functor_base<void>
{
	IC	void		operator()	() const
	{
		functr( parameter );
	}
};

template <>
struct functor2<void> : public functor<void>
{
	LPCSTR			parameter2;
	IC	void		operator()	() const
	{
		functr( parameter, parameter2 );
	}
};

template <>
struct functor3<void> : public functor2<void>
{
	int				parameter3;
	IC	void		operator()	() const
	{
		functr( parameter, parameter2, parameter3 );
	}
};

} // namespace detail

enum EMaxProps
{
	max_properties_count = 3,
};

class Upgrade : public UpgradeBase
{
private:
	typedef		UpgradeBase	inherited;
public:
							Upgrade();
	virtual					~Upgrade();
				void		construct( const shared_str& upgrade_id, Group& parental_group, Manager& manager_r );

	IC			LPCSTR		section() const;
	IC shared_str const&	parent_group_id() const;
	IC Group const*			parent_group() const;
	IC			LPCSTR		icon_name() const;
	IC			LPCSTR		name() const;
	IC			LPCSTR		description_text() const;

				LPCSTR		get_prerequisites();
		UpgradeStateResult	get_preconditions();
	IC			bool		get_highlight() const;
	IC	shared_str const&	get_property_name(u8 index=0) const;
	IC	Ivector2 const&		get_scheme_index() const;

#ifdef DEBUG
	virtual		void		log_hierarchy( LPCSTR nest );
#endif // DEBUG

	virtual		void		fill_root_container( Root* root );

	virtual		UpgradeStateResult		can_install( CInventoryItem& item, bool loading );
				bool		check_scheme_index( const Ivector2& scheme_index );
				void		set_highlight( bool value );
				void		run_effects( bool loading );

	virtual		void		highlight_up();
	virtual		void		highlight_down();

protected:
	typedef detail::functor<bool>			BoolFunctor;
	typedef detail::functor2<bool>			BoolFunctor2;
	typedef detail::functor<void>			VoidFunctor;
	typedef detail::functor2<void>			VoidFunctor2;
	typedef detail::functor3<void>			VoidFunctor3;

	typedef detail::functor2<LPCSTR>		StrFunctor;
	typedef detail::functor2<int>			IntFunctor;

protected:
	Group*					m_parent_group;

	shared_str				m_section;
	Ivector2				m_scheme_index;

	shared_str				m_name;
	shared_str				m_description;
	shared_str				m_icon;
	shared_str				m_properties[max_properties_count];

	IntFunctor				m_preconditions;
	VoidFunctor3			m_effects;
	StrFunctor				m_prerequisites;
//	VoidFunctor			m_tooltip;

	bool					m_highlight;

};

} // namespace upgrade
} // namespace inventory

#include "inventory_upgrade_inline.h"

#endif // INVENTORY_UPGRADE_H_INCLUDED
