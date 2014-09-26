// Boost token_iterator.hpp  -------------------------------------------------//

// Copyright John R. Bandela 2001
// Permission to copy, use, modify, sell and distribute this software
// is granted provided this copyright notice appears in all
// copies. This software is provided "as is" without express or
// implied warranty, and with no claim as to its suitability for any
// purpose.

// See http://www.boost.org/libs/tokenizer for documentation.

#ifndef BOOST_TOKENIZER_POLICY_JRB051801_HPP_
#define BOOST_TOKENIZER_POLICY_JRB051801_HPP_

#include<boost/iterator_adaptors.hpp>
#include<boost/token_functions.hpp>
#include<utility>
#include<cassert>

namespace boost {
    namespace detail{
        // The base "iterator" for iterator adapter
        template<class It>
        class token_iterator_base
        {
        public:
            std::pair<It,It> p_;
            bool valid_;
            token_iterator_base():p_(It(),It()),valid_(false){}
            token_iterator_base(const It& b , const It& e )
                :p_(b,e),valid_(false){}
            operator It(){return p_.first;}
            
            template<class T>
            token_iterator_base(const token_iterator_base<T>& other)
                :p_(other.p_),valid_(other.valid_){}
        };
        
        
        template<class Type, class TokenizerFunc>
        class tokenizer_policy{
        private:
            TokenizerFunc func_;
            Type tok_;
        public:
            tokenizer_policy(){}
            tokenizer_policy(const TokenizerFunc& f):func_(f){};
            
            template<class Base>
            void initialize(Base& b){
                if(b.valid_) return;
                func_.reset();
                b.valid_ = (b.p_.first != b.p_.second)?
                    func_(b.p_.first,b.p_.second,tok_):false;
            }
            
            template<class Iterator1, class Iterator2>
                bool equal(const Iterator1& a, const Iterator2& b) const{
                return (a.base().valid_ && b.base().valid_)
                    ?(a.base().p_==b.base().p_)
                    :(a.base().valid_==b.base().valid_);
                
            }
            
            template<class Iterator>
                typename Iterator::reference
                dereference(const Iterator& a) const{
                using namespace std;
                assert(a.base().valid_);
                return tok_;
            }   
            template <class Iterator>
                void increment(Iterator& b){
                using namespace std;
                assert(b.base().valid_);
                b.base().valid_ = func_(b.base().p_.first,b.base().p_.second,tok_);
            }
            
        };
        
    } // namespace detail

    template <
        class TokenizerFunc = char_delimiters_separator<char>, 
        class Iterator = std::string::const_iterator,
        class Type = std::string
    >
    class token_iterator_generator {

    private: 
        typedef Type value_type;
        typedef detail::tokenizer_policy<Type, TokenizerFunc> policies;
        typedef detail::token_iterator_base<Iterator> base;
        typedef typename boost::detail::non_bidirectional_category<
            Iterator>::type category;
    public:
        typedef boost::iterator_adaptor<base,policies,value_type, 
            const value_type&,const value_type*,category,std::ptrdiff_t> type;
    };
    
    
    // Type has to be first because it needs to be explicitly specified
    // because there is no way the function can deduce it.
    template<class Type, class Iterator, class TokenizerFunc>
        typename token_iterator_generator<TokenizerFunc,Iterator,Type>::type 
    make_token_iterator(Iterator begin, Iterator end,const TokenizerFunc& fun){
        typedef typename 
            token_iterator_generator<TokenizerFunc,Iterator,Type>::type ret_type;
        detail::token_iterator_base<Iterator> b(begin,end);
        detail::tokenizer_policy<Type,TokenizerFunc> f(fun);
        return ret_type(b,f);
    }

} // namespace boost

#endif
