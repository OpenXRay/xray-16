////////////////////////////////////////////////////////////////////////////
//	Module 		: buffer_vector.h
//	Created 	: 10.10.2007
//  Modified 	: 10.10.2007
//	Author		: Dmitriy Iassenev
//	Description : buffer vector template class
////////////////////////////////////////////////////////////////////////////

#ifndef BUFFER_VECTOR_H_INCLUDED
#define BUFFER_VECTOR_H_INCLUDED

template <typename T>
class buffer_vector {
public:
	typedef T const *								const_iterator;
	typedef T const *								const_pointer;
	typedef T const &								const_reference;
	typedef std::reverse_iterator<const_iterator>	const_reverse_iterator;

	typedef T *										iterator;
	typedef T *										pointer;
	typedef T &										reference;
	typedef std::reverse_iterator<iterator>			reverse_iterator;

	typedef ptrdiff_t								difference_type;
	typedef size_t									size_type;
	typedef T										value_type;

private:
	typedef buffer_vector<T>						self_type;

public:
	inline							buffer_vector	(void *buffer, size_type const &max_count);
	inline							buffer_vector	(void *buffer, size_type const &max_count, size_type const &count, value_type const &value);
	inline							buffer_vector	(void *buffer, size_type const &max_count, self_type const &other);
	template <typename input_iterator>
	inline							buffer_vector	(void *buffer, size_type const &max_count, input_iterator const &begin, input_iterator const &last);
	inline							~buffer_vector	();

	inline	self_type				&operator=		(self_type const &other);

	template <typename input_iterator>
	inline	void					assign			(input_iterator begin, input_iterator const &end);
	inline	void					assign			(size_type const &count, const_reference value);

	inline	void					swap			(self_type &other);
	inline	void					clear			();
	inline	void					resize			(size_type const &size);
	inline	void					reserve			(size_type const &size);

	template <typename input_iterator>
	inline	void					insert			(iterator const &where, input_iterator begin, input_iterator const &last);
	inline	void					insert			(iterator const &where, size_type const &count, const_reference value);
	inline	void					insert			(iterator const &where, const_reference value);

	inline	void					erase			(iterator const &begin, iterator const &end);
	inline	void					erase			(iterator const &where);

	inline	void					pop_back		();
	inline	void					push_back		(const_reference value);

	inline	reference				at				(size_type const &index);
	inline	const_reference			at				(size_type const &index) const;

	inline	reference				operator[]		(size_type const &index);
	inline	const_reference			operator[]		(size_type const &index) const;

	inline	reference				back			();
	inline	const_reference			back			() const;

	inline	reference				front			();
	inline	const_reference			front			() const;

	inline	iterator				begin			();
	inline	const_iterator			begin			() const;

	inline	iterator				end				();
	inline	const_iterator			end				() const;

	inline	reverse_iterator		rbegin			();
	inline	const_reverse_iterator	rbegin			() const;

	inline	reverse_iterator		rend			();
	inline	const_reverse_iterator	rend			() const;

	inline	bool					empty			() const;
	inline	size_t					size			() const;

	inline	size_t					capacity		() const;
	inline	size_t					max_size		() const;

private:
	static inline void				construct		(pointer p);
	static inline void				construct		(pointer p, const_reference value);
	static inline void				construct		(iterator begin, iterator const &end);
	static inline void				construct		(iterator begin, iterator const &end, const_reference value);

private:
	static inline void				destroy			(pointer p);
	static inline void				destroy			(iterator begin, iterator const &end);

private:
	pointer							m_begin;
	pointer							m_end;
	pointer							m_max_end;
};

	template <typename T>
	inline	void					swap			(buffer_vector<T> &left, buffer_vector<T> &right);

#include "buffer_vector_inline.h"

#endif // BUFFER_VECTOR_H_INCLUDED