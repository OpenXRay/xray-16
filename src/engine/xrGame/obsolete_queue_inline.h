#include "../xrCore/buffer_vector.h"

template<typename T, unsigned int MaxCount>
class obsolete_queue<buffer_vector<T>, MaxCount>
{
public:
	typedef buffer_vector<T>							container_type;
	typedef typename container_type::value_type			value_type;
	typedef typename container_type::reference			reference;
	typedef typename container_type::const_reference	const_reference;
	typedef typename container_type::iterator			iterator;
	typedef typename container_type::const_iterator		const_iterator;
	typedef typename container_type::size_type			size_type;
	
							obsolete_queue	() : m_sequence(m_sequence_store, MaxCount) {};
							~obsolete_queue	() {};
		
	void					push_obsolete	(const_reference element)
	{
		if (m_sequence.size() >= MaxCount)
		{
			m_sequence.erase(m_sequence.begin());
		}
		m_sequence.push_back(element);
	}

	const_iterator			begin			() const { return m_sequence.begin(); };
	const_iterator			end				() const { return m_sequence.end(); };
	iterator				begin			() { return m_sequence.begin(); };
	iterator				end				() { return m_sequence.end(); };
	void					clear			() { m_sequence.clear(); };

	size_type				size			() const { return m_sequence.size(); };
	container_type const &	get_contaier	() const { return m_sequence; };
private:
	container_type			m_sequence;
	T						m_sequence_store[MaxCount];
}; //class obsolete_queue<buffer_vector<T>, MaxCount>
