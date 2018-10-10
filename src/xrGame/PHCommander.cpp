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
        call = NULL;
    }
}
/////////////////////////////////////////////////////////////////////////////////
CPHCommander::~CPHCommander() { clear(); }
void CPHCommander::clear()
{
    while (m_calls.size())
    {
        remove_call(m_calls.end() - 1);
    }
}

void CPHCommander::update()
{
    for (u32 i = 0; i < m_calls.size(); i++)
    {
        try
        {
            m_calls[i]->check();
        }
        catch (...)
        {
            remove_call(m_calls.begin() + i);
            i--;
            continue;
        }

        if (m_calls[i]->obsolete())
        {
            remove_call(m_calls.begin() + i);
            i--;
            continue;
        }
    }
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

void CPHCommander::remove_call(PHCALL_I i)
{
#ifdef DEBUG
    const CPHCallOnStepCondition* esc = smart_cast<const CPHCallOnStepCondition*>((*i)->condition());
    const CPHConstForceAction* cfa = smart_cast<const CPHConstForceAction*>((*i)->action());
    if (esc && cfa)
    {
        Fvector f = cfa->force();
        float m = f.magnitude();
        if (m > EPS_S)
            f.mul(1.f / m);
        // Msg(" const force removed: force: %f,  remove step: %d  world step: %d ,dir(%f,%f,%f) ", m, esc->step(),
        // (u32)physics_world()->StepsNum(), f.x, f.y , f.z );
    }
#endif
    delete_call(*i);
    m_calls.erase(i);
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

struct SRemoveRped
{
    CPHReqComparerV* cmp_object;
    SRemoveRped(CPHReqComparerV* cmp_o) { cmp_object = cmp_o; }
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
    m_calls.erase(std::remove_if(m_calls.begin(), m_calls.end(), SRemoveRped(cmp_object)), m_calls.end());
}

void CPHCommander::phys_shell_relcase(CPhysicsShell* sh)
{
    CPHReqComparerHasShell c(sh);
    remove_calls_threadsafety(&c);
}
