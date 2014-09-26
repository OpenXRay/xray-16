//
//  Copyright (c) 2000-2002
//  Joerg Walter, Mathias Koch
//
//  Permission to use, copy, modify, distribute and sell this software
//  and its documentation for any purpose is hereby granted without fee,
//  provided that the above copyright notice appear in all copies and
//  that both that copyright notice and this permission notice appear
//  in supporting documentation.  The authors make no representations
//  about the suitability of this software for any purpose.
//  It is provided "as is" without express or implied warranty.
//
//  The authors gratefully acknowledge the support of
//  GeNeSys mbH & Co. KG in producing this work.
//

#ifndef BOOST_UBLAS_OPERATION_BLOCKED_H
#define BOOST_UBLAS_OPERATION_BLOCKED_H

namespace boost { namespace numeric { namespace ublas {

    template<class V, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prod (const matrix_expression<E1> &e1,
                const vector_expression<E2> &e2,
                std::size_t block_size) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;

        V v (e1 ().size1 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        indexing_vector_assign (scalar_assign<value_type, value_type> (), cv, prod (e1, e2));
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
            vector_range<vector_type> v_range (v, range (i_begin, i_end));
            v_range.assign (zero_vector<value_type> (i_end - i_begin));
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
                const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (j_begin, j_end));
                const vector_range<expression2_type> e2_range (e2 (), range (j_begin, j_end));
                v_range.plus_assign (prod (e1_range, e2_range));
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (equals (v, cv), internal_logic ());
#endif
        return v;
    }

    template<class V, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prec_prod (const matrix_expression<E1> &e1,
                     const vector_expression<E2> &e2,
                     std::size_t block_size) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;

        V v (e1 ().size1 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        indexing_vector_assign (scalar_assign<value_type, value_type> (), cv, prod (e1, e2));
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
            vector_range<vector_type> v_range (v, range (i_begin, i_end));
            v_range.assign (zero_vector<value_type> (i_end - i_begin));
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
                const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (j_begin, j_end));
                const vector_range<expression2_type> e2_range (e2 (), range (j_begin, j_end));
                v_range.plus_assign (prec_prod (e1_range, e2_range));
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (equals (v, cv), internal_logic ());
#endif
        return v;
    }

    template<class V, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prod (const vector_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                std::size_t block_size) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;

        V v (e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        indexing_vector_assign (scalar_assign<value_type, value_type> (), cv, prod (e1, e2));
#endif
        size_type i_size = BOOST_UBLAS_SAME (e1 ().size (), e2 ().size1 ());
        size_type j_size = e2 ().size2 ();
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
            vector_range<vector_type> v_range (v, range (j_begin, j_end));
            v_range.assign (zero_vector<value_type> (j_end - j_begin));
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
                const vector_range<expression1_type> e1_range (e1 (), range (i_begin, i_end));
                const matrix_range<expression2_type> e2_range (e2 (), range (i_begin, i_end), range (j_begin, j_end));
                v_range.plus_assign (prod (e1_range, e2_range));
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (equals (v, cv), internal_logic ());
#endif
        return v;
    }

    template<class V, class E1, class E2>
    BOOST_UBLAS_INLINE
    V
    block_prec_prod (const vector_expression<E1> &e1,
                     const matrix_expression<E2> &e2,
                     std::size_t block_size) {
        typedef V vector_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename V::size_type size_type;
        typedef typename V::value_type value_type;

        V v (e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        vector<value_type> cv (v.size ());
        indexing_vector_assign (scalar_assign<value_type, value_type> (), cv, prod (e1, e2));
#endif
        size_type i_size = BOOST_UBLAS_SAME (e1 ().size (), e2 ().size1 ());
        size_type j_size = e2 ().size2 ();
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
            vector_range<vector_type> v_range (v, range (j_begin, j_end));
            v_range.assign (zero_vector<value_type> (j_end - j_begin));
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
                const vector_range<expression1_type> e1_range (e1 (), range (i_begin, i_end));
                const matrix_range<expression2_type> e2_range (e2 (), range (i_begin, i_end), range (j_begin, j_end));
                v_range.plus_assign (prec_prod (e1_range, e2_range));
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        BOOST_UBLAS_CHECK (equals (v, cv), internal_logic ());
#endif
        return v;
    }

    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                std::size_t block_size,
                row_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        M m (e1 ().size1 (), e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, row_major> cm (m.size1 (), m.size2 ());
        indexing_matrix_assign (scalar_assign<value_type, value_type> (), cm, prod (e1, e2), row_major_tag ());
        disable_type_check = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + std::min (k_size - k_begin, block_size);
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
                    m_range.plus_assign (prod (e1_range, e2_range));
                }
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        disable_type_check = false;
        BOOST_UBLAS_CHECK (equals (m, cm), internal_logic ());
#endif
        return m;
    }
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prec_prod (const matrix_expression<E1> &e1,
                     const matrix_expression<E2> &e2,
                     std::size_t block_size,
                     row_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        M m (e1 ().size1 (), e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, row_major> cm (m.size1 (), m.size2 ());
        indexing_matrix_assign (scalar_assign<value_type, value_type> (), cm, prod (e1, e2), row_major_tag ());
        disable_type_check = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
            size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
            for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
                size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + std::min (k_size - k_begin, block_size);
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
                    m_range.plus_assign (prec_prod (e1_range, e2_range));
                }
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        disable_type_check = false;
        BOOST_UBLAS_CHECK (equals (m, cm), internal_logic ());
#endif
        return m;
    }

    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                std::size_t block_size,
                column_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        M m (e1 ().size1 (), e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, column_major> cm (m.size1 (), m.size2 ());
        indexing_matrix_assign (scalar_assign<value_type, value_type> (), cm, prod (e1, e2), column_major_tag ());
        disable_type_check = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + std::min (k_size - k_begin, block_size);
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
                    m_range.plus_assign (prod (e1_range, e2_range));
                }
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        disable_type_check = false;
        BOOST_UBLAS_CHECK (equals (m, cm), internal_logic ());
#endif
        return m;
    }
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prec_prod (const matrix_expression<E1> &e1,
                     const matrix_expression<E2> &e2,
                     std::size_t block_size,
                     column_major_tag) {
        typedef M matrix_type;
        typedef const E1 expression1_type;
        typedef const E2 expression2_type;
        typedef typename M::size_type size_type;
        typedef typename M::value_type value_type;

        M m (e1 ().size1 (), e2 ().size2 ());
#ifdef BOOST_UBLAS_TYPE_CHECK
        matrix<value_type, column_major> cm (m.size1 (), m.size2 ());
        indexing_matrix_assign (scalar_assign<value_type, value_type> (), cm, prod (e1, e2), column_major_tag ());
        disable_type_check = true;
#endif
        size_type i_size = e1 ().size1 ();
        size_type j_size = e2 ().size2 ();
        size_type k_size = BOOST_UBLAS_SAME (e1 ().size2 (), e2 ().size1 ());
        for (size_type j_begin = 0; j_begin < j_size; j_begin += block_size) {
            size_type j_end = j_begin + std::min (j_size - j_begin, block_size);
            for (size_type i_begin = 0; i_begin < i_size; i_begin += block_size) {
                size_type i_end = i_begin + std::min (i_size - i_begin, block_size);
                matrix_range<matrix_type> m_range (m, range (i_begin, i_end), range (j_begin, j_end));
                m_range.assign (zero_matrix<value_type> (i_end - i_begin, j_end - j_begin));
                for (size_type k_begin = 0; k_begin < k_size; k_begin += block_size) {
                    size_type k_end = k_begin + std::min (k_size - k_begin, block_size);
                    const matrix_range<expression1_type> e1_range (e1 (), range (i_begin, i_end), range (k_begin, k_end));
                    const matrix_range<expression2_type> e2_range (e2 (), range (k_begin, k_end), range (j_begin, j_end));
                    m_range.plus_assign (prec_prod (e1_range, e2_range));
                }
            }
        }
#ifdef BOOST_UBLAS_TYPE_CHECK
        disable_type_check = false;
        BOOST_UBLAS_CHECK (equals (m, cm), internal_logic ());
#endif
        return m;
    }

    // Dispatcher
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prod (const matrix_expression<E1> &e1,
                const matrix_expression<E2> &e2,
                std::size_t block_size) {
        typedef typename M::orientation_category orientation_category;
        return block_prod<M> (e1, e2, block_size, orientation_category ());
    }
    template<class M, class E1, class E2>
    BOOST_UBLAS_INLINE
    M
    block_prec_prod (const matrix_expression<E1> &e1,
                     const matrix_expression<E2> &e2,
                     std::size_t block_size) {
        typedef typename M::orientation_category orientation_category;
        return block_prec_prod<M> (e1, e2, block_size, orientation_category ());
    }

}}}

#endif


