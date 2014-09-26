// Boost.Signals library
//
// Copyright (C) 2001-2002 Doug Gregor (gregod@cs.rpi.edu)
//
// Permission to copy, use, sell and distribute this software is granted
// provided this copyright notice appears in all copies.
// Permission to modify the code and to distribute modified code is granted
// provided this copyright notice appears in all copies, and a notice
// that the code was modified is included with the copyright notice.
//
// This software is provided "as is" without express or implied warranty,
// and with no claim as to its suitability for any purpose.
 
// For more information, see http://www.boost.org

#ifndef BOOST_SIGNALS_SLOT_CALL_ITERATOR
#define BOOST_SIGNALS_SLOT_CALL_ITERATOR

#include <functional>
#include <boost/iterator_adaptors.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/signals/connection.hpp>

namespace boost {
  namespace BOOST_SIGNALS_NAMESPACE {
    namespace detail {
      // A cached return value from a slot
      template<typename T>
      struct cached_return_value {
        cached_return_value(const T& t) : value(t) {}
        
        T value;
      };

      // Generates a slot call iterator. Essentially, this is an iterator that:
      //   - skips over disconnected slots in the underlying list
      //   - calls the connected slots when dereferenced
      //   - caches the result of calling the slots
      template<typename Function, typename Iterator>
      class slot_call_policies : public default_iterator_policies {
      public:
        typedef typename Function::result_type result_type;

        slot_call_policies() {}

        slot_call_policies(const Iterator& x, Function fi) :
          end(x), f(fi), cache()
        {
        }
        
        void initialize(Iterator& x)
        { 
          x = std::find_if(x, end, std::not1(is_disconnected()));
          cache.reset();
        }
        
        template <class IteratorAdaptor>
        typename IteratorAdaptor::reference 
        dereference(const IteratorAdaptor& x) const
        {
          if (!cache.get()) {
            cache.reset(new cached_return_value<result_type>(f(*x.base())));
          }
          
          return cache->value;
        }

        template<typename IteratorAdaptor>
        void increment(IteratorAdaptor& x)
        {
          ++x.base();
          x.base() = std::find_if(x.base(), x.policies().end, 
                                  std::not1(is_disconnected()));
          cache.reset();
        }
        
        template<typename IteratorAdaptor1, typename IteratorAdaptor2>
        bool equal(const IteratorAdaptor1& x, const IteratorAdaptor2& y) const
        {
          Iterator xb = std::find_if(x.base(), x.policies().end, 
                                     std::not1(is_disconnected()));
          Iterator yb = std::find_if(y.base(), y.policies().end, 
                                     std::not1(is_disconnected()));
          const_cast<IteratorAdaptor1&>(x).base() = xb;
          const_cast<IteratorAdaptor1&>(y).base() = yb;
          return xb == yb; 
        }
        
      private:
        Iterator end;
        Function f;
        mutable shared_ptr< cached_return_value<result_type> > cache;
      };

      template<typename Function, typename Iterator>
      class slot_call_iterator_generator {
      private:
        typedef typename Function::result_type value_type;
      public:
        typedef slot_call_policies<Function, Iterator> policy_type;
        typedef iterator_adaptor<Iterator, policy_type, value_type,
                                 value_type&, value_type*, 
                                 std::input_iterator_tag> type;
      };

      template<typename Function, typename Iterator>
      inline typename slot_call_iterator_generator<Function, Iterator>::type
      make_slot_call_iterator(Iterator first, Iterator last, Function f)
      {
        typedef slot_call_iterator_generator<Function, Iterator> gen;
        typedef typename gen::type sc_iterator;
        typedef typename gen::policy_type sc_policy;

        return sc_iterator(first, sc_policy(last, f));
      }
    } // end namespace detail
  } // end namespace BOOST_SIGNALS_NAMESPACE
} // end namespace boost
#endif // BOOST_SIGNALS_SLOT_CALL_ITERATOR
