#pragma once

#include "xrCommon/xr_smart_pointers.h"
#include "xrCommon/xr_vector.h"
#include "xrCommon/xr_array.h"
#include "xrCore/Threading/Lock.hpp"
#include "xrCore/Threading/ScopeLock.hpp"

class CEventNotifierCallback
{
public:
    using CID = size_t;
    static const CID INVALID_CID = std::numeric_limits<CID>::max();

    virtual void ProcessEvent() = 0;
    virtual ~CEventNotifierCallback(){};
};

template <unsigned int CNT>
class CEventNotifier
{
private:
    class CCallbackStorage
    {
        class CCallbackWrapper
        {
        public:
            xr_unique_ptr<CEventNotifierCallback> callback;
            bool destroying = false;
            bool executing = false;

            CCallbackWrapper(CEventNotifierCallback* cb) : callback(cb){};
            bool operator==(const CEventNotifierCallback* cb) { return cb == callback.get(); }
            void Reset()
            {
                callback.reset(nullptr);
                destroying = false;
                executing = false;
            }
        };
        xr_vector<CCallbackWrapper> m_callbacks;
        Lock m_lock;

    public:
        CEventNotifierCallback::CID RegisterCallback(CEventNotifierCallback* cb)
        {
            ScopeLock lock(&m_lock);
            auto it = std::find(m_callbacks.begin(), m_callbacks.end(), nullptr);
            return (it == m_callbacks.end()) ? (m_callbacks.emplace_back(cb), m_callbacks.size() - 1) :
                                               (it->callback.reset(cb), std::distance(m_callbacks.begin(), it));
        }

        bool UnregisterCallback(CEventNotifierCallback::CID cid)
        {
            bool result = false;
            ScopeLock lock(&m_lock);
            if (cid < m_callbacks.size() && m_callbacks[cid].callback != nullptr)
            {
                if (!m_callbacks[cid].destroying)
                {
                    result = true;
                    m_callbacks[cid].destroying = true;
                }

                if (!m_callbacks[cid].executing)
                {
                    m_callbacks[cid].Reset();
                }
            }
            return result;
        }

        void ExecuteCallbacks()
        {
            ScopeLock lock(&m_lock);
            for (CEventNotifierCallback::CID i = 0; i < m_callbacks.size(); ++i)
            {
                auto& cb = m_callbacks[i];
                if (cb.callback != nullptr && !cb.destroying)
                {
                    cb.executing = true;
                    cb.callback->ProcessEvent();
                    cb.executing = false;

                    if (cb.destroying)
                    {
                        UnregisterCallback(i);
                    }
                }
            }
        }
    };

    xr_array<CCallbackStorage, CNT> m_callbacks;

public:
    CEventNotifierCallback::CID RegisterCallback(CEventNotifierCallback* cb, unsigned int event_id)
    {
        R_ASSERT(event_id < CNT);
        return m_callbacks[event_id].RegisterCallback(cb);
    }

    bool UnregisterCallback(CEventNotifierCallback::CID cid, unsigned int event_id)
    {
        R_ASSERT(event_id < CNT);
        return m_callbacks[event_id].UnregisterCallback(cid);
    }

    void FireEvent(unsigned int event_id)
    {
        R_ASSERT(event_id < CNT);
        m_callbacks[event_id].ExecuteCallbacks();
    }
};
