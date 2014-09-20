#ifndef OBSOLETE_QUEUE_INCLUDED
#define OBSOLETE_QUEUE_INCLUDED

template<typename Contaiter, unsigned int MaxCount>
class obsolete_queue
{
public:
	typedef Contaiter								container_type;
	typedef typename Contaiter::value_type			value_type;
	typedef typename Contaiter::reference			reference;
	typedef typename Contaiter::const_reference		const_reference;
	typedef typename Contaiter::iterator			iterator;
	typedef typename Contaiter::const_iterator		const_iterator;
	typedef typename Contaiter::size_type			size_type;
	
							obsolete_queue	();
							~obsolete_queue	();
		
	void					push_obsolete	(const_reference element);
	const_iterator			begin			() const;
	const_iterator			end				() const;
	iterator				begin			();
	iterator				end				();
	void					clear			();
	size_type				size			() const;
	container_type const &	get_contaier	() const;
};

#include "obsolete_queue_inline.h"

#endif //#ifndef OBSOLETE_QUEUE_INCLUDED