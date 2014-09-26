// This code is in the public domain -- castanyo@yahoo.es

#ifndef NV_CORE_CONTAINER_H
#define NV_CORE_CONTAINER_H

/*
These containers are based on Thatcher Ulrich <tu@tulrich.com> containers,
donated to the Public Domain.

I've also borrowed some ideas from the Qt toolkit, specially the cool
foreach iterator.

TODO
Do not use memmove in insert & remove, use copy ctors instead.
*/


// nvcore
#include <nvcore/nvcore.h>
#include <nvcore/Memory.h>
#include <nvcore/Debug.h>

#include <string.h>	// memmove
#include <new>		// for placement new


#if NV_CC_GNUC // If typeof is available:

#define NV_FOREACH(i, container) \
	typedef typeof(container) NV_STRING_JOIN2(cont,__LINE__); \
	for(NV_STRING_JOIN2(cont,__LINE__)::PseudoIndex i((container).start()); !(container).isDone(i); (container).advance(i))
/*
#define NV_FOREACH(i, container) \
	for(typename typeof(container)::PseudoIndex i((container).start()); !(container).isDone(i); (container).advance(i))
*/

#else // If typeof not available:

struct PseudoIndexWrapper {
	template <typename T>
	PseudoIndexWrapper(const T & container) {
		nvStaticCheck(sizeof(typename T::PseudoIndex) <= sizeof(memory));
		new (memory) typename T::PseudoIndex(container.start());
	}
	// PseudoIndex cannot have a dtor!

	template <typename T> typename T::PseudoIndex & operator()(const T * container) {
		return *reinterpret_cast<typename T::PseudoIndex *>(memory);
	}
	template <typename T> const typename T::PseudoIndex & operator()(const T * container) const {
		return *reinterpret_cast<const typename T::PseudoIndex *>(memory);
	}

	uint8 memory[4];	// Increase the size if we have bigger enumerators.
};

#define NV_FOREACH(i, container) \
	for(PseudoIndexWrapper i(container); !(container).isDone(i(&(container))); (container).advance(i(&(container))))

#endif

// Declare foreach keyword.
#if !defined NV_NO_USE_KEYWORDS
#	define foreach NV_FOREACH
#endif



namespace nv 
{
	// Templates

	/// Return the maximum of two values.
	template <typename T> 
	inline const T & max(const T & a, const T & b)
	{
		//return std::max(a, b);
		if( a < b ) {
			return b; 
		}
		return a;
	}
	
	/// Return the minimum of two values.
	template <typename T> 
	inline const T & min(const T & a, const T & b)
	{
		//return std::min(a, b);
		if( b < a ) {
			return b; 
		}
		return a;
	}
	
	/// Clamp between two values.
	template <typename T> 
	inline const T & clamp(const T & x, const T & a, const T & b)
	{
		return min(max(x, a), b);
	}
	
	/// Swap two values.
	template <typename T> 
	inline void swap(T & a, T & b)
	{
		//return std::swap(a, b);
		T temp = a; 
		a = b; 
		b = temp;
	}

	template <typename Key> struct hash 
	{
		inline uint sdbm_hash(const void * data_in, uint size, uint h = 5381)
		{
			const uint8 * data = (const uint8 *) data_in;
			uint i = 0;
			while (i < size) {
				h = (h << 16) + (h << 6) - h + (uint) data[i++];
			}
			return h;
		}
		
		uint operator()(const Key & k) {
			return sdbm_hash(&k, sizeof(Key));
		}
	};
	template <> struct hash<int>
	{
		uint operator()(int x) const { return x; }
	};
	template <> struct hash<uint>
	{
		uint operator()(uint x) const { return x; }
	};
	
	/// Delete all the elements of a container.
	template <typename T>
	void deleteAll(T & container)
	{
		for(typename T::PseudoIndex i = container.start(); !container.isDone(i); container.advance(i))
		{
			delete container[i];
		}
	}

	
	/** Return the next power of two. 
	* @see http://graphics.stanford.edu/~seander/bithacks.html
	* @warning Behaviour for 0 is undefined.
	* @note isPowerOfTwo(x) == true -> nextPowerOfTwo(x) == x
	* @note nextPowerOfTwo(x) = 2 << log2(x-1)
	*/
	inline uint nextPowerOfTwo( uint x )
	{
		nvDebugCheck( x != 0 );
	#if 1	// On modern CPUs this is as fast as using the bsr instruction.
		x--;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		return x+1;	
	#else
		uint p = 1;
		while( x > p ) {
			p += p;
		}
		return p;
	#endif
	}

	/// Return true if @a n is a power of two.
	inline bool isPowerOfTwo( uint n )
	{
		return (n & (n-1)) == 0;
	}

	/// Simple iterator interface.
	template <typename T>
	struct Iterator
	{
		virtual void advance();
		virtual bool isDone();
		virtual T current();
	};


	/**
	* Replacement for std::vector that is easier to debug and provides
	* some nice foreach enumerators. 
	*/
	template<typename T>
	class NVCORE_CLASS Array {
	public:
		
		/// Ctor.
		Array() : m_buffer(NULL), m_size(0), m_buffer_size(0)
		{
		}
	
		/// Copy ctor.
		Array( const Array & a ) : m_buffer(NULL), m_size(0), m_buffer_size(0)
		{
			copy(a.m_buffer, a.m_size); 
		}
	
		/// Ctor that initializes the vector with the given elements.
		Array( const T * ptr, int num ) : m_buffer(NULL), m_size(0), m_buffer_size(0)
		{
			copy(ptr, num);
		}
	
		/// Allocate array.
		explicit Array(uint capacity) : m_buffer(NULL), m_size(0), m_buffer_size(0)
		{
			allocate(capacity);
		}
		
	
		/// Dtor.
		~Array()
		{
			clear();
			allocate(0);
		}
	
	
		/// Const and save vector access.
		const T & operator[]( uint index ) const
		{
			nvDebugCheck(index < m_size);
			return m_buffer[index];
		}
	
		/// Safe vector access.
		T & operator[] ( uint index )
		{
			nvDebugCheck(index < m_size);
			return m_buffer[index];
		}
	
	
		/// Get vector size.
		uint size() const { return m_size; }
		
		/// Get vector size.
		uint count() const { return m_size; }
	
		/// Get const vector pointer.
		const T * buffer() const { return m_buffer; }
	
		/// Get vector pointer.
		T * unsecureBuffer() { return m_buffer; }
	
		/// Is vector empty.
		bool isEmpty() const { return m_size == 0; }
	
		/// Is a null vector.
		bool isNull() const	{ return m_buffer == NULL; }
	
	
		/// Push an element at the end of the vector.
		void push_back( const T & val )
		{
			uint new_size = m_size + 1;

			if (new_size > m_buffer_size)
			{
				const T copy(val);	// create a copy in case value is inside of this array.
				resize(new_size);
				m_buffer[new_size-1] = copy;
			}
			else
			{
				m_size = new_size;
				new(m_buffer+new_size-1) T(val);
			}
		}
		void pushBack( const T & val )
		{
			push_back(val);
		}
		void append( const T & val )
		{
			push_back(val);
		}
		
		/// Qt like push operator.
		Array<T> & operator<< ( T & t )
		{
			push_back(t);
			return *this;
		}
		
		/// Pop and return element at the end of the vector.
		void pop_back()
		{
			nvDebugCheck( m_size > 0 );
			resize( m_size - 1 );
		}
		void popBack()
		{
			pop_back();
		}
		
		/// Get back element.
		const T & back() const
		{
			nvDebugCheck( m_size > 0 );
			return m_buffer[m_size-1];
		}
		
		/// Get back element.
		T & back()
		{
			nvDebugCheck( m_size > 0 );
			return m_buffer[m_size-1];
		}
		
		/// Get front element.
		const T & front() const
		{
			nvDebugCheck( m_size > 0 );
			return m_buffer[0];
		}
		
		/// Get front element.
		T & front()
		{
			nvDebugCheck( m_size > 0 );
			return m_buffer[0];
		}
		
		/// Check if the given element is contained in the array.
		bool contains(const T & e) const
		{
			for (uint i = 0; i < m_size; i++) {
				if (m_buffer[i] == e) return true;
			}
			return false;
		}
		
		/// Remove the element at the given index. This is an expensive operation!
		void removeAt( uint index )
		{
			nvCheck(index >= 0 && index < m_size);
			
			if( m_size == 1 ) {
				clear();
			}
			else {
				m_buffer[index].~T();
				
				memmove( m_buffer+index, m_buffer+index+1, sizeof(T) * (m_size - 1 - index) );
				m_size--;
			}
		}
		
		/// Remove the first instance of the given element.
		void remove(const T & element)
		{
			for(PseudoIndex i = start(); !isDone(i); advance(i)) {
				removeAt(i);
				break;
			}
		}
		
		/// Insert the given element at the given index shifting all the elements up.
		void insertAt( uint index, const T & val = T() )
		{
			nvCheck( index >= 0 && index <= m_size );
			
			resize( m_size + 1 );
			
			if( index < m_size - 1 ) {
				memmove( m_buffer+index+1, m_buffer+index, sizeof(T) * (m_size - 1 - index) );
			}
			
			// Copy-construct into the newly opened slot.
			new(m_buffer+index) T(val);
		}
		
		/// Append the given data to our vector.
		void append(const Array<T> & other)
		{
			append(other.m_buffer, other.m_size);
		}
		
		/// Append the given data to our vector.
		void append(const T other[], uint count)
		{
			if( count > 0 ) {
				const uint old_size = m_size;
				resize(m_size + count);
				// Must use operator=() to copy elements, in case of side effects (e.g. ref-counting).
				for( uint i = 0; i < count; i++ ) {
					m_buffer[old_size + i] = other[i];
				}
			}
		}
		
		
		/// Remove the given element by replacing it with the last one.
		void replaceWithLast(uint index)
		{
			nvDebugCheck( index < m_size );
			m_buffer[index] = back();
			(m_buffer+m_size-1)->~T();
			m_size--;
		}
	
	
		/// Resize the vector preserving existing elements.
		void resize(uint new_size)
		{
			uint i;
			uint old_size = m_size;
			m_size = new_size;
			
			// Destruct old elements (if we're shrinking).
			for( i = new_size; i < old_size; i++ ) {
				(m_buffer+i)->~T();							// Explicit call to the destructor
			}
			
			if( m_size == 0 ) {
				//Allocate(0);	// Don't shrink automatically.
			}
			else if( m_size <= m_buffer_size/* && m_size > m_buffer_size >> 1*/) {
				// don't compact yet.
				nvDebugCheck(m_buffer != NULL);
			}
			else {
				uint new_buffer_size;
				if( m_buffer_size == 0 ) {
					// first allocation
					new_buffer_size = m_size;
				}
				else {
					// growing
					new_buffer_size = m_size + (m_size >> 2);
				}
				allocate( new_buffer_size );
			}
			
			// Call default constructors
			for( i = old_size; i < new_size; i++ ) {
				new(m_buffer+i) T;	// placement new
			}
		}
	
	
		/// Resize the vector preserving existing elements and initializing the
		/// new ones with the given value.
		void resize( uint new_size, const T &elem )
		{
			uint i;
			uint old_size = m_size;
			m_size = new_size;
			
			// Destruct old elements (if we're shrinking).
			for( i = new_size; i < old_size; i++ ) {
				(m_buffer+i)->~T();							// Explicit call to the destructor
			}
			
			if( m_size == 0 ) {
				//Allocate(0);	// Don't shrink automatically.
			}
			else if( m_size <= m_buffer_size && m_size > m_buffer_size >> 1 ) {
				// don't compact yet.
			}
			else {
				uint new_buffer_size;
				if( m_buffer_size == 0 ) {
					// first allocation
					new_buffer_size = m_size;
				}
				else {
					// growing
					new_buffer_size = m_size + (m_size >> 2);
				}
				allocate( new_buffer_size );
			}
			
			// Call copy constructors
			for( i = old_size; i < new_size; i++ ) {
				new(m_buffer+i) T( elem );	// placement new
			}
		}
		
		/// Tighten the memory used by the container.
		void tighten()
		{
			// TODO Reallocate only if worth.
		}
		
		/// Clear the buffer.
		void clear()
		{
			resize(0);
		}
		
		/// Shrink the allocated vector.
		void shrink()
		{
			if( m_size < m_buffer_size ) {
				allocate(m_size);
			}
		}
		
		/// Preallocate space.
		void reserve(uint desired_size)
		{
			if( desired_size > m_buffer_size ) {
				allocate( desired_size );
			}
		}
		
		/// Copy memory to our vector. Resizes the vector if needed.
		void copy( const T * ptr, uint num )
		{
			resize( num );
			for(uint i = 0; i < m_size; i++) {
				m_buffer[i] = ptr[i];
			}
		}
		
		/// Assignment operator.
		void operator=( const Array<T> & a )
		{
			copy( a.m_buffer, a.m_size );
		}
		
		/*
		/// Array serialization.
		friend Stream & operator<< ( Stream & s, Array<T> & p )
		{
			if( s.isLoading() ) {
				uint size;
				s << size;
				p.resize( size );
			}
			else {
				s << p.m_size;
			}
			
			for( uint i = 0; i < p.m_size; i++ ) {
				s << p.m_buffer[i];
			}
			
			return s;
		}
		*/
	
		// Array enumerator.
		typedef uint PseudoIndex;
		
		PseudoIndex start() const { return 0; }
		bool isDone(const PseudoIndex & i) const { nvDebugCheck(i <= this->m_size); return i == this->m_size; };
		void advance(PseudoIndex & i) const { nvDebugCheck(i <= this->m_size); i++; }
		
	#if NV_CC_MSVC
		T & operator[]( const PseudoIndexWrapper & i ) {
			return m_buffer[i(this)];
		}
		const T & operator[]( const PseudoIndexWrapper & i ) const {
			return m_buffer[i(this)];		
		}
	#endif
	
	
		/// Swap the members of this vector and the given vector.
		friend void swap(Array<T> & a, Array<T> & b)
		{
			swap(a.m_buffer, b.m_buffer);
			swap(a.m_size, b.m_size);
			swap(a.m_buffer_size, b.m_buffer_size);
		}
	
	
	private:
	
		/// Change buffer size.
		void allocate( uint rsize )
		{
			m_buffer_size = rsize;
			
			// free the buffer.
			if( m_buffer_size == 0 ) {
				if( m_buffer ) {
					mem::free( m_buffer );
					m_buffer = NULL;
				}
			}
			
			// realloc the buffer
			else {
				if( m_buffer ) m_buffer = (T *) mem::realloc( m_buffer, sizeof(T) * m_buffer_size );
				else m_buffer = (T *) mem::malloc( sizeof(T) * m_buffer_size );
			}
		}
		
		
	private:
		T * m_buffer;
		uint m_size;
		uint m_buffer_size;
	};



	/** Thatcher Ulrich's hash table.
	*
	* Hash table, linear probing, internal chaining.  One
	* interesting/nice thing about this implementation is that the table
	* itself is a flat chunk of memory containing no pointers, only
	* relative indices.  If the key and value types of the hash contain
	* no pointers, then the hash can be serialized using raw IO.  Could
	* come in handy.
	*
	* Never shrinks, unless you explicitly clear() it.  Expands on
	* demand, though.  For best results, if you know roughly how big your
	* table will be, default it to that size when you create it.
	*/
	template<typename T, typename U, typename hash_functor = hash<T> >
	class NVCORE_CLASS HashMap
	{
		NV_FORBID_COPY(HashMap)
	public:

		/// Default ctor.
		HashMap() : entry_count(0), size_mask(-1), table(NULL) { }

		/// Ctor with size hint.
		explicit HashMap(int size_hint) : entry_count(0), size_mask(-1), table(NULL) { setCapacity(size_hint); }

		/// Dtor.
		~HashMap() { clear(); }
	
	
		/// Set a new or existing value under the key, to the value.
		void set(const T& key, const U& value)
		{
			int	index = findIndex(key);
			if (index >= 0)
			{
				E(index).value = value;
				return;
			}
			
			// Entry under key doesn't exist.
			add(key, value);
		}
		
	
		/// Add a new value to the hash table, under the specified key.
		void add(const T& key, const U& value)
		{
			nvCheck(findIndex(key) == -1);
			
			checkExpand();
			nvCheck(table != NULL);
			entry_count++;
			
			const uint hash_value = hash_functor()(key);
			const int index = hash_value & size_mask;
			
			Entry * natural_entry = &(E(index));
			
			if (natural_entry->isEmpty())
			{
				// Put the new entry in.
				new (natural_entry) Entry(key, value, -1, hash_value);
			}
			else
			{
				// Find a blank spot.
				int	blank_index = index;
				for (;;)
				{
					blank_index = (blank_index + 1) & size_mask;
					if (E(blank_index).isEmpty()) break;	// found it
				}
				Entry * blank_entry = &E(blank_index);
				
				if (int(natural_entry->hash_value & size_mask) == index)
				{
					// Collision.  Link into this chain.
					
					// Move existing list head.
					new (blank_entry) Entry(*natural_entry);	// placement new, copy ctor
					
					// Put the new info in the natural entry.
					natural_entry->key = key;
					natural_entry->value = value;
					natural_entry->next_in_chain = blank_index;
					natural_entry->hash_value = hash_value;
				}
				else
				{
					// Existing entry does not naturally
					// belong in this slot.  Existing
					// entry must be moved.
					
					// Find natural location of collided element (i.e. root of chain)
					int	collided_index = natural_entry->hash_value & size_mask;
					for (;;)
					{
						Entry * e = &E(collided_index);
						if (e->next_in_chain == index)
						{
							// Here's where we need to splice.
							new (blank_entry) Entry(*natural_entry);
							e->next_in_chain = blank_index;
							break;
						}
						collided_index = e->next_in_chain;
						nvCheck(collided_index >= 0 && collided_index <= size_mask);
					}
					
					// Put the new data in the natural entry.
					natural_entry->key = key;
					natural_entry->value = value;
					natural_entry->hash_value = hash_value;
					natural_entry->next_in_chain = -1;
				}
			}
		}
	
	
		/// Remove the first value under the specified key.
		bool remove(const T& key)
		{
			if (table == NULL)
			{
				return false;
			}
			
			int	index = findIndex(key);
			if (index < 0)
			{
				return false;
			}
			
			Entry * entry = &E(index);
			
			if( entry->isEndOfChain() ) {
				entry->clear();
			}
			else {
				// Get next entry.
				Entry & next_entry = E(entry->next_in_chain);
				
				// Copy next entry in this place.
				new (entry) Entry(next_entry);
				
				next_entry.clear();
			}
			
			entry_count--;
			
			return true;
		}
		
	
		/// Remove all entries from the hash table.
		void clear()
		{
			if (table != NULL)
			{
				// Delete the entries.
				for (int i = 0, n = size_mask; i <= n; i++)
				{
					Entry * e = &E(i);
					if (e->isEmpty() == false)
					{
						e->clear();
					}
				}
				mem::free(table);
				table = NULL;
				entry_count = 0;
				size_mask = -1;
			}
		}
	
		
		/// Returns true if the hash is empty.
		bool isEmpty() const
		{
			return table == NULL || entry_count == 0;
		}
	
	
		/** Retrieve the value under the given key.
		 *
		 * If there's no value under the key, then return false and leave
		 * *value alone.
		 *
		 * If there is a value, return true, and set *value to the entry's
		 * value.
		 *
		 * If value == NULL, return true or false according to the
		 * presence of the key, but don't touch *value.
		 */
		bool get(const T& key, U* value = NULL) const
		{
			int	index = findIndex(key);
			if (index >= 0)
			{
				if (value) {
					*value = E(index).value;	// take care with side-effects!
				}
				return true;
			}
			return false;
		}
		
		/// Determine if the given key is contained in the hash.
		bool contains(const T & key) const
		{
			return get(key);
		}
	
		/// Number of entries in the hash.
		int	size() const
		{
			return entry_count;
		}
	
		/// Number of entries in the hash.
		int	count() const
		{
			return size();
		}
		
	
		/**
		* Resize the hash table to fit one more entry.  Often this
		* doesn't involve any action.
		*/
		void checkExpand()
		{
			if (table == NULL) {
				// Initial creation of table.  Make a minimum-sized table.
				setRawCapacity(16);
			} 
			else if (entry_count * 3 > (size_mask + 1) * 2) {
				// Table is more than 2/3rds full.  Expand.
				setRawCapacity(entry_count * 2);
			}
		}
	
	
		/// Hint the bucket count to >= n.
		void resize(int n)
		{
			// Not really sure what this means in relation to
			// STLport's hash_map... they say they "increase the
			// bucket count to at least n" -- but does that mean
			// their real capacity after resize(n) is more like
			// n*2 (since they do linked-list chaining within
			// buckets?).
			setCapacity(n);
		}
	
		/**
		* Size the hash so that it can comfortably contain the given
		* number of elements.  If the hash already contains more
		* elements than new_size, then this may be a no-op.
		*/
		void setCapacity(int new_size)
		{
			int	new_raw_size = (new_size * 3) / 2;
			if (new_raw_size < size()) { return; }
			
			setRawCapacity(new_raw_size);
		}
	
		/// Behaves much like std::pair.
		struct Entry
		{
			int	next_in_chain;	// internal chaining for collisions
			uint hash_value;	// avoids recomputing.  Worthwhile?
			T key;
			U value;
			
			Entry() : next_in_chain(-2) {}
			Entry(const Entry& e)
				: next_in_chain(e.next_in_chain), hash_value(e.hash_value), key(e.key), value(e.value)
			{
			}
			Entry(const T& k, const U& v, int next, int hash)
				: next_in_chain(next), hash_value(hash), key(k), value(v)
			{
			}
			bool isEmpty() const { return next_in_chain == -2; }
			bool isEndOfChain() const { return next_in_chain == -1; }
			
			void clear()
			{
				key.~T();	// placement delete
				value.~U();	// placement delete
				next_in_chain = -2;
			}
		};
	
		
		// HashMap enumerator.
		typedef int PseudoIndex;
		PseudoIndex start() const { PseudoIndex i = 0; findNext(i); return i; }
		bool isDone(const PseudoIndex & i) const { nvDebugCheck(i <= size_mask+1); return i == size_mask+1; };
		void advance(PseudoIndex & i) const { nvDebugCheck(i <= size_mask+1); i++; findNext(i); }
		
	#if NV_CC_GNUC
		Entry & operator[]( const PseudoIndex & i ) {
			return E(i);
		}
		const Entry & operator[]( const PseudoIndex & i ) const {
			return E(i);
		}
	#elif NV_CC_MSVC
		Entry & operator[]( const PseudoIndexWrapper & i ) {
			return E(i(this));
		}
		const Entry & operator[]( const PseudoIndexWrapper & i ) const {
			return E(i(this));
		}
	#endif
		
		
		
	private:
	
		// Find the index of the matching entry. If no match, then return -1.
		int	findIndex(const T& key) const
		{
			if (table == NULL) return -1;
			
			uint hash_value = hash_functor()(key);
			int	index = hash_value & size_mask;
			
			const Entry * e = &E(index);
			if (e->isEmpty()) return -1;
			if (int(e->hash_value & size_mask) != index) return -1;	// occupied by a collider
			
			for (;;)
			{
				nvCheck((e->hash_value & size_mask) == (hash_value & size_mask));
				
				if (e->hash_value == hash_value && e->key == key)
				{
					// Found it.
					return index;
				}
				nvDebugCheck(! (e->key == key));	// keys are equal, but hash differs!
				
				// Keep looking through the chain.
				index = e->next_in_chain;
				if (index == -1) break;	// end of chain
				
				nvCheck(index >= 0 && index <= size_mask);
				e = &E(index);
				
				nvCheck(e->isEmpty() == false);
			}
			return -1;
		}
	
		// Helpers.
		Entry & E(int index)
		{
			nvDebugCheck(table != NULL);
			nvDebugCheck(index >= 0 && index <= size_mask);
			return table[index];
		}
		const Entry & E(int index) const
		{
			nvDebugCheck(table != NULL);
			nvDebugCheck(index >= 0 && index <= size_mask);
			return table[index];
		}
	
		
		/**
		 * Resize the hash table to the given size (Rehash the
		 * contents of the current table).  The arg is the number of
		 * hash table entries, not the number of elements we should
		 * actually contain (which will be less than this).
		 */
		void setRawCapacity(int new_size)
		{
			if (new_size <= 0) {
				// Special case.
				clear();
				return;
			}
			
			// Force new_size to be a power of two.
			new_size = nextPowerOfTwo(new_size);
			
			HashMap<T, U, hash_functor> new_hash;
			new_hash.table = (Entry *) mem::malloc(sizeof(Entry) * new_size);
			nvDebugCheck(new_hash.table != NULL);
			
			new_hash.entry_count = 0;
			new_hash.size_mask = new_size - 1;
			for (int i = 0; i < new_size; i++)
			{
				new_hash.E(i).next_in_chain = -2;	// mark empty
			}
			
			// Copy stuff to new_hash
			if (table != NULL)
			{
				for (int i = 0, n = size_mask; i <= n; i++)
				{
					Entry * e = &E(i);
					if (e->isEmpty() == false)
					{
						// Insert old entry into new hash.
						new_hash.add(e->key, e->value);
						e->clear();	// placement delete of old element
					}
				}
				
				// Delete our old data buffer.
				mem::free(table);
			}
			
			// Steal new_hash's data.
			entry_count = new_hash.entry_count;
			size_mask = new_hash.size_mask;
			table = new_hash.table;
			new_hash.entry_count = 0;
			new_hash.size_mask = -1;
			new_hash.table = NULL;
		}
	
		// Move the enumerator to the next valid element.
		void findNext(PseudoIndex & i) const {
			while (i <= size_mask && E(i).isEmpty()) {
				i++;
			}
		}
		
		
		int	entry_count;
		int	size_mask;
		Entry * table;
	
	};



} // nv namespace

#endif // NV_CORE_CONTAINER_H
