Executing ssh-askpass to query the password...
Warning: Remote host denied X11 forwarding, perhaps xauth program could not be run on the server side.
// (C) Copyright Jeremy Siek 2001. Permission to copy, use, modify,
// sell and distribute this software is granted provided this
// copyright notice appears in all copies. This software is provided
// "as is" without express or implied warranty, and with no claim as
// to its suitability for any purpose.

#include <boost/config.hpp>

// UNDER CONSTRUCTION

namespace boost {

  namespace detail {

    template <typename SizeType>
    struct intset_iterator_policies : public default_iterator_policies
    {
      intset_iterator_policies(const SizeType* value_ptr)
        : m_value(value_ptr) { }

      SizeType dereference(type<SizeType>, const SizeType& x) {
        return m_value[x];
      }
      SizeType* m_value;
    };
    
    template <typename SizeType, 
              typename Allocator = std::allocator<SizeType> >
    class intset
    {
      typedef intset self;
      typedef intset_iterator_policies<size_type> policies_t;
    public:
      typedef SizeType size_type;

      typedef typename iterator_adaptor<size_type,
        policies_t, size_type, size_type, size_type*,
        std::input_iterator_tag, std::ptrdiff_t> iterator;
      typedef iterator const_iterator;

      intset(size_type n, const Allocator alloc = Allocator)
        : m_index(n + 1, alloc), m_value(n + 1, alloc), m_next(0)
        { }
      
      bool test(size_type pos) const {
        BOOST_ASSERT_THROW
          (pos < size(), std::out_of_range("boost::intset::test(pos)"));
        if (m_index[pos] < m_next && m_index[x] >= 0)
          if (m_value[index[x]] == x)
            return true;
        return false;
      }

      self& set(size_type pos, int val = true)
      {
        BOOST_ASSERT_THROW
          (pos < size(), std::out_of_range("boost::intset::set(pos,val)"));
        if (!test(pos)) {
          ++m_next;
          m_value[m_next] = pos;
          m_index[pos] = m_next;
        }
        return *this;
      }

      self& reset(size_type pos) {
        BOOST_ASSERT_THROW
          (pos < size(), std::out_of_range("boost::intset::reset(pos)"));
        if (test(pos)) {
          m_value[m_index[pos]] = m_value[m_next];
          m_index[m_value[m_next]] = m_index[x];
          ++m_next;
        }
      }
      
      iterator begin() const { 
        return iterator(0, policies_t(&m_value[0])); 
      }
      iterator end() const {
        return iterator(m_next, policies_t(&m_value[0])); 
      }
      
    private:
      std::vector<size_type, Allocator> m_index;
      std::vector<size_type, Allocator> m_value;
      size_type m_next;
    };

  } // namespace detail 

} // namespace boost
