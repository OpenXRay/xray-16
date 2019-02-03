#include "StdAfx.h"
#include "PHCommander.h"
#include "PHSimpleCalls.h"

#ifdef DEBUG
#include "xrPhysics/IPHWorld.h"
#endif

CPHCall::CPHCall(CPHCondition* condition, CPHAction* action)
{
    m_action = action;
    m_condition = condition;
}

CPHCall::~CPHCall()
{
    xr_delete(m_action);
    xr_delete(m_condition);
}
bool CPHCall::obsolete() { return !m_action || m_action->obsolete() || !m_condition || m_condition->obsolete(); }
void CPHCall::check()
{
    if (m_condition && m_condition->is_true() && m_action)
        m_action->run();
}

bool CPHCall::equal(CPHReqComparerV* cmp_condition, CPHReqComparerV* cmp_action)
{
    return m_action->compare(cmp_action) && m_condition->compare(cmp_condition);
}
bool CPHCall::is_any(CPHReqComparerV* v) { return m_action->compare(v) || m_condition->compare(v); }
void delete_call(CPHCall*& call)
{
    try
    {
        xr_delete(call);
    }
    catch (...)
    {
        call = nullptr;
    }
}
/////////////////////////////////////////////////////////////////////////////////
CPHCommander::~CPHCommander() { clear(); }
void CPHCommander::clear()
{
    for(auto& it : m_calls)
        delete_call(it);

    m_calls.clear();
    m_callsUpdateDeferred.clear();
}

void CPHCommander::UpdateDeferred()
{
    for (auto it : m_callsUpdateDeferred)
    {
        if (it.second)
        {
            m_calls.push_back(it.first);
        }
        else
        {
            auto callToDeleteIt = std::find(m_calls.begin(), m_calls.end(), it.first);
            if (callToDeleteIt != m_calls.end())
            {
                delete_call(*callToDeleteIt);
                m_calls.erase(callToDeleteIt);
            }
        }
    }

    m_callsUpdateDeferred.clear();
}

void CPHCommander::update()
{
    m_calls.erase(
        std::remove_if(m_calls.begin(), m_calls.end(), [](CPHCall* call)
    {
        try
        {
            call->check();
        }
        catch (...)
        {
            delete_call(call);
            return true;
        }

        if (call->obsolete())
        {
            delete_call(call);
            return true;
        }

        return false;
    }), m_calls.end());
}
void CPHCommander::update_threadsafety()
{
    lock.Enter();
    update();
    lock.Leave();
}

void CPHCommander::add_call_threadsafety(CPHCondition* condition, CPHAction* action)
{
    lock.Enter();
    add_call(condition, action);
    lock.Leave();
}

void CPHCommander::add_call(CPHCondition* condition, CPHAction* action)
{
    m_calls.push_back(new CPHCall(condition, action));
}

void CPHCommander::AddCallDeferred(CPHCondition* condition, CPHAction* action)
{
    m_callsUpdateDeferred.insert({new CPHCall(condition, action), true});
}

struct SFEqualPred
{
    CPHReqComparerV *cmp_condition, *cmp_action;
    SFEqualPred(CPHReqComparerV* cmp_c, CPHReqComparerV* cmp_a)
    {
        cmp_condition = cmp_c;
        cmp_action = cmp_a;
    }
    bool operator()(CPHCall* call) { return call->equal(cmp_condition, cmp_action); }
};
struct SFRemovePred2
{
    CPHReqComparerV *cmp_condition, *cmp_action;
    SFRemovePred2(CPHReqComparerV* cmp_c, CPHReqComparerV* cmp_a)
    {
        cmp_condition = cmp_c;
        cmp_action = cmp_a;
    }
    bool operator()(CPHCall* call)
    {
        if (call->equal(cmp_condition, cmp_action))
        {
            delete_call(call);
            return true;
        }
        return false;
    }
};

PHCALL_I CPHCommander::find_call(CPHReqComparerV* cmp_condition, CPHReqComparerV* cmp_action)
{
    return std::find_if(m_calls.begin(), m_calls.end(), SFEqualPred(cmp_condition, cmp_action));
}

bool CPHCommander::has_call(CPHReqComparerV* cmp_condition, CPHReqComparerV* cmp_action)
{
    return find_call(cmp_condition, cmp_action) != m_calls.end();
}

void CPHCommander::remove_call(CPHReqComparerV* cmp_condition, CPHReqComparerV* cmp_action)
{
    m_calls.erase(
        std::remove_if(m_calls.begin(), m_calls.end(), SFRemovePred2(cmp_condition, cmp_action)), m_calls.end());
}

bool CPHCommander::add_call_unique(
    CPHCondition* condition, CPHReqComparerV* cmp_condition, CPHAction* action, CPHReqComparerV* cmp_action)
{
    if (m_calls.end() == find_call(cmp_condition, cmp_action))
    {
        add_call(condition, action);
        return true;
    }
    return false;
}

struct SRemovePred1
{
    CPHReqComparerV* cmp_object;
    SRemovePred1(CPHReqComparerV* cmp_o) { cmp_object = cmp_o; }
    bool operator()(CPHCall* call)
    {
        if (call->is_any(cmp_object))
        {
            delete_call(call);
            return true;
        }
        else
            return false;
    }
};

void CPHCommander::remove_calls_threadsafety(CPHReqComparerV* cmp_object)
{
    lock.Enter();
    remove_calls(cmp_object);
    lock.Leave();
}

void CPHCommander::remove_calls(CPHReqComparerV* cmp_object)
{
    m_calls.erase(std::remove_if(m_calls.begin(), m_calls.end(), SRemovePred1(cmp_object)), m_calls.end());
}

void CPHCommander::RemoveCallsDeferred(CPHReqComparerV* comparer)
{
    for (const auto& call : m_calls)
    {
        if (call->is_any(comparer))
        {
            m_callsUpdateDeferred.insert({call, false});
        }
    }
}

void CPHCommander::phys_shell_relcase(CPhysicsShell* sh)
{
    CPHReqComparerHasShell c(sh);
    remove_calls_threadsafety(&c);
}
