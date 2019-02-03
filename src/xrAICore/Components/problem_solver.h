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

// #DARK: removed _inline.h, as @xottab_duty asked
#pragma region inline

#ifndef AI_COMPILER
#include "xrAICore/Navigation/graph_engine.h"
#include "xrAICore/Navigation/graph_engine_space.h"
#endif

#include "xrGame/ai_space.h"

#define TEMPLATE_SPECIALIZATION                                                                                  \
    template <typename _operator_condition, typename _operator, typename _condition_state,                       \
        typename _condition_evaluator, typename _operator_id_type, bool _reverse_search, typename _operator_ptr, \
        typename _condition_evaluator_ptr>

#define CProblemSolverAbstract                                                                                \
    CProblemSolver<_operator_condition, _operator, _condition_state, _condition_evaluator, _operator_id_type, \
        _reverse_search, _operator_ptr, _condition_evaluator_ptr>

TEMPLATE_SPECIALIZATION
IC CProblemSolverAbstract::CProblemSolver() { init(); }
TEMPLATE_SPECIALIZATION
CProblemSolverAbstract::~CProblemSolver() { clear(); }
TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::clear()
{
    while (!m_operators.empty())
        remove_operator(m_operators.back().m_operator_id);

    while (!m_evaluators.empty())
        remove_evaluator((*(m_evaluators.end() - 1)).first);
}

TEMPLATE_SPECIALIZATION
void CProblemSolverAbstract::init() {}
TEMPLATE_SPECIALIZATION
void CProblemSolverAbstract::setup()
{
    m_target_state.clear();
    m_current_state.clear();
    m_temp.clear();
    m_solution.clear();
    m_applied = false;
    m_solution_changed = false;
    m_actuality = true;
    m_failed = false;
}

TEMPLATE_SPECIALIZATION
IC bool CProblemSolverAbstract::actual() const
{
    if (!m_actuality)
        return (false);

    typename xr_vector<_operator_condition>::const_iterator I = current_state().conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator E = current_state().conditions().end();
    typename EVALUATORS::const_iterator i = evaluators().begin();
    typename EVALUATORS::const_iterator e = evaluators().end();
    for (; I != E; ++I)
    {
        if ((*i).first < (*I).condition())
            i = std::lower_bound(i, e, (*I).condition(), evaluators().value_comp());
        VERIFY(i != e);
        VERIFY((*i).first == (*I).condition());
        if ((*i).second->evaluate() != (*I).value())
            return (false);
    }
    return (true);
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::add_operator(const _operator_id_type& operator_id, _operator_ptr _op)
{
    typename OPERATOR_VECTOR::iterator I = std::lower_bound(m_operators.begin(), m_operators.end(), operator_id);
    THROW((I == m_operators.end()) || ((*I).m_operator_id != operator_id));
#ifdef DEBUG
    validate_properties(_op->conditions());
    validate_properties(_op->effects());
#endif
    m_actuality = false;
    m_operators.insert(I, SOperator(operator_id, _op));
}

#ifdef DEBUG
TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::validate_properties(const CState& conditions) const
{
    for (const auto& cond : conditions.conditions())
    {
        if (evaluators().find(cond.condition()) == evaluators().end())
        {
            Msg("! cannot find corresponding evaluator to the property with id %d", cond.condition());
            THROW(evaluators().find(cond.condition()) != evaluators().end());
        }
    }
}
#endif

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::remove_operator(const _operator_id_type& operator_id)
{
    typename OPERATOR_VECTOR::iterator I = std::lower_bound(m_operators.begin(), m_operators.end(), operator_id);
    THROW(m_operators.end() != I);
    try
    {
        delete_data((*I).m_operator);
    }
    catch (...)
    {
        (*I).m_operator = 0;
    }
    m_actuality = false;
    m_operators.erase(I);
}

TEMPLATE_SPECIALIZATION
IC const typename CProblemSolverAbstract::OPERATOR_VECTOR& CProblemSolverAbstract::operators() const
{
    return (m_operators);
}

// states
TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::set_target_state(const CState& state)
{
    m_actuality = m_actuality && (m_target_state == state);
    m_target_state = state;
}

TEMPLATE_SPECIALIZATION
IC const typename CProblemSolverAbstract::CState& CProblemSolverAbstract::current_state() const
{
    return (m_current_state);
}

TEMPLATE_SPECIALIZATION
IC const typename CProblemSolverAbstract::CState& CProblemSolverAbstract::target_state() const
{
    return (m_target_state);
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::add_evaluator(const condition_type& condition_id, _condition_evaluator_ptr evaluator)
{
    THROW(evaluators().end() == evaluators().find(condition_id));
    m_evaluators.insert(std::make_pair(condition_id, evaluator));
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::remove_evaluator(const condition_type& condition_id)
{
    typename EVALUATORS::iterator I = m_evaluators.find(condition_id);
    THROW(I != m_evaluators.end());
    try
    {
        delete_data((*I).second);
    }
    catch (...)
    {
        (*I).second = 0;
    }
    m_evaluators.erase(I);
    m_actuality = false;
}

TEMPLATE_SPECIALIZATION
IC _condition_evaluator_ptr CProblemSolverAbstract::evaluator(const condition_type& condition_id) const
{
    typename EVALUATORS::const_iterator I = evaluators().find(condition_id);
    THROW(evaluators().end() != I);
    return ((*I).second);
}

TEMPLATE_SPECIALIZATION
IC const typename CProblemSolverAbstract::EVALUATORS& CProblemSolverAbstract::evaluators() const
{
    return (m_evaluators);
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::evaluate_condition(typename xr_vector<_operator_condition>::const_iterator& I,
    typename xr_vector<_operator_condition>::const_iterator& E, const condition_type& condition_id) const
{
    size_t index = I - m_current_state.conditions().begin();
    m_current_state.add_condition(I, _operator_condition(condition_id, evaluator(condition_id)->evaluate()));
    I = m_current_state.conditions().begin() + index;
    E = m_current_state.conditions().end();
}

TEMPLATE_SPECIALIZATION
IC typename CProblemSolverAbstract::edge_value_type CProblemSolverAbstract::get_edge_weight(
    const _index_type& vertex_index0, const _index_type& vertex_index1, const const_iterator& i) const
{
    edge_value_type current, min;
    current = (*i).m_operator->weight(vertex_index1, vertex_index0);
    min = (*i).m_operator->min_weight();
    THROW(current >= min);
    return (current);
}

TEMPLATE_SPECIALIZATION
IC bool CProblemSolverAbstract::is_accessible(const _index_type& vertex_index) const { return (m_applied); }
TEMPLATE_SPECIALIZATION
IC const typename CProblemSolverAbstract::_index_type& CProblemSolverAbstract::value(
    const _index_type& vertex_index, const_iterator& i, bool reverse_search) const
{
    if (reverse_search)
    {
        if ((*i).m_operator->applicable_reverse(
                (*i).m_operator->effects(), (*i).m_operator->conditions(), vertex_index))
            m_applied = (*i).m_operator->apply_reverse(
                vertex_index, (*i).m_operator->effects(), m_temp, (*i).m_operator->conditions());
        else
            m_applied = false;
    }
    else
    {
        if ((*i).m_operator->applicable(vertex_index, current_state(), (*i).m_operator->conditions(), *this))
        {
            (*i).m_operator->apply(vertex_index, (*i).m_operator->effects(), m_temp, m_current_state, *this);
            m_applied = true;
        }
        else
            m_applied = false;
    }
    return (m_temp);
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::begin(const _index_type& vertex_index, const_iterator& b, const_iterator& e) const
{
    b = m_operators.begin();
    e = m_operators.end();
}

TEMPLATE_SPECIALIZATION
IC bool CProblemSolverAbstract::is_goal_reached(const _index_type& vertex_index) const
{
    return (is_goal_reached_impl<reverse_search>(vertex_index));
}

TEMPLATE_SPECIALIZATION
IC bool CProblemSolverAbstract::is_goal_reached_impl(const _index_type& vertex_index) const
{
    static_assert(!reverse_search, "This function cannot be used in the REVERSE search.");
    typename xr_vector<_operator_condition>::const_iterator I = vertex_index.conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator E = vertex_index.conditions().end();
    typename xr_vector<_operator_condition>::const_iterator i = target_state().conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator e = target_state().conditions().end();
    typename xr_vector<_operator_condition>::const_iterator II = current_state().conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator EE = current_state().conditions().end();
    for (; (i != e) && (I != E);)
    {
        if ((*I).condition() < (*i).condition())
        {
            ++I;
        }
        else if ((*I).condition() > (*i).condition())
        {
            for (; (II != EE) && ((*II).condition() < (*i).condition());)
                ++II;
            if ((II == EE) || ((*II).condition() > (*i).condition()))
                evaluate_condition(II, EE, (*i).condition());
            if ((*II).value() != (*i).value())
                return (false);
            ++II;
            ++i;
        }
        else
        {
            if ((*I).value() != (*i).value())
                return (false);
            ++I;
            ++i;
        }
    }

    if (I == E)
    {
        I = II;
        E = EE;
    }
    else
        return (true);

    for (; i != e;)
    {
        if ((I == E) || ((*I).condition() > (*i).condition()))
            evaluate_condition(I, E, (*i).condition());

        if ((*I).condition() < (*i).condition())
            ++I;
        else
        {
            VERIFY((*I).condition() == (*i).condition());
            if ((*I).value() != (*i).value())
                return (false);
            ++I;
            ++i;
        }
    }
    return (true);
}

TEMPLATE_SPECIALIZATION
IC bool CProblemSolverAbstract::is_goal_reached_impl(const _index_type& vertex_index, bool) const
{
    static_assert(reverse_search, "This function cannot be used in the STRAIGHT search.");
    typename xr_vector<_operator_condition>::const_iterator I = m_current_state.conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator E = m_current_state.conditions().end();
    typename xr_vector<_operator_condition>::const_iterator i = vertex_index.conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator e = vertex_index.conditions().end();
    for (; i != e;)
    {
        if ((I == E) || ((*I).condition() > (*i).condition()))
            evaluate_condition(I, E, (*i).condition());

        if ((*I).condition() < (*i).condition())
            ++I;
        else
        {
            if ((*I).value() != (*i).value())
                return (false);
            ++I;
            ++i;
        }
    }
    return (true);
}

TEMPLATE_SPECIALIZATION
IC const xr_vector<_operator_id_type>& CProblemSolverAbstract::solution() const { return (m_solution); }

TEMPLATE_SPECIALIZATION
IC _operator_ptr CProblemSolverAbstract::get_operator(const _operator_id_type& operator_id)
{
    typename OPERATOR_VECTOR::iterator I = std::lower_bound(m_operators.begin(), m_operators.end(), operator_id);
    THROW(m_operators.end() != I);
    return ((*I).get_operator());
}

TEMPLATE_SPECIALIZATION
IC void CProblemSolverAbstract::solve()
{
#ifndef AI_COMPILER
    m_solution_changed = false;

    if (actual())
        return;

    m_actuality = true;
    m_solution_changed = true;
    m_current_state.clear();

    m_failed = !ai().graph_engine().search(*this, reverse_search ? target_state() : current_state(),
        reverse_search ? current_state() : target_state(), &m_solution,
        GraphEngineSpace::CSolverBaseParameters(
            GraphEngineSpace::_solver_dist_type(-1), GraphEngineSpace::_solver_condition_type(-1), 8000));
#endif
}

TEMPLATE_SPECIALIZATION
IC typename CProblemSolverAbstract::edge_value_type CProblemSolverAbstract::estimate_edge_weight(
    const _index_type& condition) const
{
    return (helper::template estimate_edge_weight_impl<reverse_search>(*this, condition));
}

TEMPLATE_SPECIALIZATION
IC typename CProblemSolverAbstract::edge_value_type CProblemSolverAbstract::estimate_edge_weight_impl(
    const _index_type& condition) const
{
    static_assert(!reverse_search, "This function cannot be used in the REVERSE search.");
    edge_value_type result = 0;
    typename xr_vector<_operator_condition>::const_iterator I = target_state().conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator E = target_state().conditions().end();
    typename xr_vector<_operator_condition>::const_iterator i = condition.conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator e = condition.conditions().end();
    for (; (I != E) && (i != e);)
        if ((*I).condition() < (*i).condition())
        {
            ++result;
            ++I;
        }
        else if ((*I).condition() > (*i).condition())
            ++i;
        else
        {
            if ((*I).value() != (*i).value())
                ++result;
            ++I;
            ++i;
        }
    return (result + edge_value_type(E - I));
}

TEMPLATE_SPECIALIZATION
IC typename CProblemSolverAbstract::edge_value_type CProblemSolverAbstract::estimate_edge_weight_impl(
    const _index_type& condition, bool) const
{
    static_assert(reverse_search, "This function cannot be used in the STRAIGHT search.");
    edge_value_type result = 0;
    typename xr_vector<_operator_condition>::const_iterator I = current_state().conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator E = current_state().conditions().end();
    typename xr_vector<_operator_condition>::const_iterator i = condition.conditions().begin();
    typename xr_vector<_operator_condition>::const_iterator e = condition.conditions().end();
    for (; (i != e);)
    {
        if ((I == E) || ((*I).condition() > (*i).condition()))
            evaluate_condition(I, E, (*i).condition());

        if ((*I).condition() < (*i).condition())
            ++I;
        else
        {
            VERIFY((*I).condition() == (*i).condition());
            if ((*I).value() != (*i).value())
                ++result;
            ++I;
            ++i;
        }
    }
    return (result);
}

#undef TEMPLATE_SPECIALIZATION
#undef CProblemSolverAbstract

#pragma endregion
