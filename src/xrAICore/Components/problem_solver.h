////////////////////////////////////////////////////////////////////////////
//	Module 		: problem_solver.h
//	Created 	: 24.02.2004
//  Modified 	: 10.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Problem solver
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "xrCore/Containers/AssociativeVector.hpp"
#include "Common/object_broker.h"

template <typename _operator_condition, typename _condition_state, typename _operator, typename _condition_evaluator,
    typename _operator_id_type, bool _reverse_search = false, typename _operator_ptr = _operator*,
    typename _condition_evaluator_ptr = _condition_evaluator*>
class CProblemSolver
{
public:
    enum
    {
        reverse_search = _reverse_search,
    };

private:
    typedef CProblemSolver<_operator_condition, _condition_state, _operator, _condition_evaluator, _operator_id_type,
        _reverse_search, _operator_ptr, _condition_evaluator_ptr>
        self_type;

public:
    typedef _operator COperator;
    typedef _condition_state CState;
    typedef _condition_evaluator CConditionEvaluator;
    typedef _operator_ptr operator_ptr;
#ifdef WINDOWS
    typedef _condition_evaluator_ptr _condition_evaluator_ptr;
#endif
    typedef typename _operator_condition::condition_type condition_type;
    typedef typename _operator_condition::value_type value_type;
    typedef typename _operator::edge_value_type edge_value_type;
    typedef CState _index_type;
    typedef _operator_id_type edge_type;

    struct SOperator
    {
        _operator_id_type m_operator_id;
        _operator_ptr m_operator;

        IC SOperator(const _operator_id_type& operator_id, _operator_ptr _op)
            : m_operator_id(operator_id), m_operator(_op)
        {
        }

        bool operator<(const _operator_id_type& operator_id) const { return (m_operator_id < operator_id); }
        _operator_ptr get_operator() const { return (m_operator); }
    };
    typedef xr_vector<SOperator> OPERATOR_VECTOR;
    typedef typename OPERATOR_VECTOR::const_iterator const_iterator;
    typedef AssociativeVector<condition_type, _condition_evaluator_ptr> EVALUATORS;

protected:
    OPERATOR_VECTOR m_operators;
    EVALUATORS m_evaluators;
    xr_vector<_operator_id_type> m_solution;
    CState m_target_state;
    mutable CState m_current_state;
    mutable CState m_temp;
    mutable bool m_applied;
    bool m_actuality;
    bool m_solution_changed;
    bool m_failed;

private:
    template <bool a>
    IC bool is_goal_reached_impl(std::enable_if_t<!a, const _index_type&> vertex_index) const
    {
        return is_goal_reached_impl(vertex_index);
    }
    template <bool a>
    IC bool is_goal_reached_impl(std::enable_if_t<a, const _index_type&> vertex_index) const
    {
        return is_goal_reached_impl(vertex_index, true);
    }

    IC bool is_goal_reached_impl(const _index_type& vertex_index) const;
    IC bool is_goal_reached_impl(const _index_type& vertex_index, bool) const;

    IC edge_value_type estimate_edge_weight_impl(const _index_type& vertex_index) const;
    IC edge_value_type estimate_edge_weight_impl(const _index_type& vertex_index, bool) const;

private:
    struct helper
    {
        template <bool a>
        static IC edge_value_type estimate_edge_weight_impl(std::enable_if_t<!a, self_type const&> self, const _index_type& vertex_index)
        {
            return self.estimate_edge_weight_impl(vertex_index);
        }

        template <bool a>
        static IC edge_value_type estimate_edge_weight_impl(std::enable_if_t<a, self_type const&> self, const _index_type& vertex_index)
        {
            return self.estimate_edge_weight_impl(vertex_index, true);
        }
    }; // struct helper

protected:
#ifdef DEBUG
    IC void validate_properties(const CState& conditions) const;
#endif

public:
    // common interface
    IC CProblemSolver();
    virtual ~CProblemSolver();
    void init();
    virtual void setup();
    IC bool actual() const;

    // graph interface
    IC edge_value_type get_edge_weight(
        const _index_type& vertex_index0, const _index_type& vertex_index1, const const_iterator& i) const;
    IC bool is_accessible(const _index_type& vertex_index) const;
    IC const _index_type& value(const _index_type& vertex_index, const_iterator& i, bool reverse_search) const;
    IC void begin(const _index_type& vertex_index, const_iterator& b, const_iterator& e) const;
    IC bool is_goal_reached(const _index_type& vertex_index) const;
    IC edge_value_type estimate_edge_weight(const _index_type& vertex_index) const;

    // operator interface
    IC virtual void add_operator(const _operator_id_type& operator_id, _operator_ptr _op);
    IC virtual void remove_operator(const _operator_id_type& operator_id);
    IC _operator_ptr get_operator(const _operator_id_type& operator_id);
    IC const OPERATOR_VECTOR& operators() const;

    // state interface
    IC void set_target_state(const CState& state);
    IC const CState& current_state() const;
    IC const CState& target_state() const;

    // evaluator interface
    IC virtual void add_evaluator(const condition_type& condition_id, _condition_evaluator_ptr evaluator);
    IC virtual void remove_evaluator(const condition_type& condition_id);
    IC _condition_evaluator_ptr evaluator(const condition_type& condition_id) const;
    IC const EVALUATORS& evaluators() const;
    IC void evaluate_condition(typename xr_vector<_operator_condition>::const_iterator& I,
        typename xr_vector<_operator_condition>::const_iterator& E, const condition_type& condition_id) const;

    // solver interface
    IC void solve();
    IC const xr_vector<_operator_id_type>& solution() const;
    virtual void clear();
};

#include "xrAICore/Components/problem_solver_inline.h"
