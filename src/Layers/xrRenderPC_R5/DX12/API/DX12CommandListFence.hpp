/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

#include "DX12.hpp"

#define CMDQUEUE_GRAPHICS 0
#define CMDQUEUE_COPY 1
#define CMDQUEUE_NUM 2

#define CMDTYPE_READ            0
#define CMDTYPE_WRITE           1
#define CMDTYPE_ANY             2
#define CMDTYPE_NUM             3

namespace std
{
inline u64 ( &max(const u64 (&a)[CMDQUEUE_NUM], const u64 (&b)[CMDQUEUE_NUM], u64 (&c)[CMDQUEUE_NUM]))[CMDQUEUE_NUM]
{
    c[CMDQUEUE_COPY    ] = max(a[CMDQUEUE_COPY    ], b[CMDQUEUE_COPY    ]);
    c[CMDQUEUE_GRAPHICS] = max(a[CMDQUEUE_GRAPHICS], b[CMDQUEUE_GRAPHICS]);

    return c;
}
}

namespace DX12
{
    inline u64 MaxFenceValue(std::atomic<u64>& a, const u64& b)
    {
        u64 utilizedValue = b;
        u64 previousValue = a;
        while (previousValue < utilizedValue &&
               !a.compare_exchange_weak(previousValue, utilizedValue))
        {
            ;
        }

        return previousValue;
    }

    inline u64 (& MaxFenceValues(const std::atomic<u64> (&a)[CMDQUEUE_NUM], const std::atomic<u64> (&b)[CMDQUEUE_NUM], u64 (&c)[CMDQUEUE_NUM]))[CMDQUEUE_NUM]
    {
        c[CMDQUEUE_COPY    ] = std::max(a[CMDQUEUE_COPY    ], b[CMDQUEUE_COPY    ]);
        c[CMDQUEUE_GRAPHICS] = std::max(a[CMDQUEUE_GRAPHICS], b[CMDQUEUE_GRAPHICS]);

        return c;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class CommandListFence
    {
    public:
        CommandListFence(Device* device);
        ~CommandListFence();

        bool Init();

        inline ID3D12Fence* GetFence() const
        {
            return m_pFence;
        }

        inline u64 GetCurrentValue() const
        {
            return m_CurrentValue;
        }

        inline void SetCurrentValue(u64 fenceValue)
        {
#ifdef DX12_IN_ORDER_ACQUIRATION
            DX12_ASSERT(m_CurrentValue <= fenceValue, "Setting new fence value which is older than the current!");
            // We do not allow smaller fences being submitted, and fences always submit in-order, no max() neccessary
            m_CurrentValue = fenceValue;
#else
            // CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_CurrentValue, fenceValue);
#endif // !NDEBUG
        }

        inline bool IsCompleted(u64 fenceValue) const
        {
            // Check against last known completed value first to avoid unnecessary fence read
            return
                (m_LastCompletedValue >= fenceValue) || (AdvanceCompletion() >= fenceValue);
        }

        void WaitForFence(u64 fenceValue);

        inline u64 AdvanceCompletion() const
        {
            // Check current completed fence
            u64 currentCompletedValue = m_pFence->GetCompletedValue();

            if (m_LastCompletedValue < currentCompletedValue)
            {
                DX12_LOG("Completed fence value(s): %d to %d", m_LastCompletedValue + 1, currentCompletedValue);
            }

#ifdef DX12_IN_ORDER_TERMINATION
            DX12_ASSERT(m_LastCompletedValue <= currentCompletedValue, "Getting new fence value which is older than the last!");
            // We do not allow smaller fences being submitted, and fences always complete in-order, no max() neccessary
            m_LastCompletedValue = currentCompletedValue;
#else
            // CLs may terminate in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_LastCompletedValue, currentCompletedValue);
#endif // !NDEBUG

            return currentCompletedValue;
        }

        inline u64 GetLastCompletedFenceValue() const
        {
            return m_LastCompletedValue;
        }

    private:
        Device* m_pDevice;
        ID3D12Fence* m_pFence;
        HANDLE m_FenceEvent;

        std::atomic<u64> m_CurrentValue;
        mutable std::atomic<u64> m_LastCompletedValue;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class CommandListFenceSet
    {
    public:
        CommandListFenceSet(Device* device);
        ~CommandListFenceSet();

        bool Init();

        inline ID3D12Fence** GetD3D12Fences()
        {
            return m_pFences;
        }

        inline ID3D12Fence* GetD3D12Fence(const int id) const
        {
            return m_pFences[id];
        }

        inline UINT64 GetSignalledValue(const int id)
        {
            return m_SignalledValues[id];
        }

        inline void SetSignalledValue(const UINT64 fenceValue, const int id)
        {
#ifdef DX12_IN_ORDER_SUBMISSION
            DX12_ASSERT(m_SignalledValues[id] <= fenceValue, "Setting new fence which is older than the signalled!");
            // We do not allow smaller fences being signalled, and fences always signal in-order, no max() neccessary
            m_SignalledValues[id] = fenceValue;
#else
            // CLs may signal in any order. Is it higher than last known signalled fence? If so, update it!
            MaxFenceValue(m_SignalledValues[id], fenceValue);
#endif
        }

        inline u64 GetSubmittedValue(const int id) const
        {
            return m_SubmittedValues[id];
        }

        inline void SetSubmittedValue(const u64 fenceValue, const int id)
        {
#ifdef DX12_IN_ORDER_SUBMISSION
            DX12_ASSERT(m_SubmittedValues[id] <= fenceValue, "Setting new fence value which is older than the submitted!");
            // We do not allow smaller fences being submitted, and fences always submit in-order, no max() neccessary
            m_SubmittedValues[id] = fenceValue;
#else
            // CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_SubmittedValues[id], fenceValue);
#endif // !NDEBUG
        }

        inline u64 GetCurrentValue(const int id) const
        {
            return m_CurrentValues[id];
        }

        inline void GetCurrentValues(u64 (&fenceValues)[CMDQUEUE_NUM]) const
        {
            fenceValues[CMDQUEUE_COPY    ] = m_CurrentValues[CMDQUEUE_COPY    ];
            fenceValues[CMDQUEUE_GRAPHICS] = m_CurrentValues[CMDQUEUE_GRAPHICS];
        }

        inline const std::atomic<u64> (& GetCurrentValues() const)[CMDQUEUE_NUM]
        {
            return m_CurrentValues;
        }

        // thread-save
        inline void SetCurrentValue(const u64 fenceValue, const int id)
        {
#ifdef DX12_IN_ORDER_ACQUIRATION
            DX12_ASSERT(m_CurrentValues[id] <= fenceValue, "Setting new fence value which is older than the current!");
            // We do not allow smaller fences being submitted, and fences always submit in-order, no max() neccessary
            m_CurrentValues[id] = fenceValue;
#else
            // CLs may submit in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_CurrentValues[id], fenceValue);
#endif // !NDEBUG
        }

        inline bool IsCompleted(const u64 fenceValue, const int id) const
        {
            // Check against last known completed value first to avoid unnecessary fence read
            return
                (m_LastCompletedValues[id] >= fenceValue) || (AdvanceCompletion(id) >= fenceValue);
        }

        inline bool IsCompleted(const u64 (&fenceValues)[CMDQUEUE_NUM]) const
        {
            // Check against last known completed value first to avoid unnecessary fence read
            return
                // TODO: return mask of completed fences so we don't check all three of them all the time
                ((m_LastCompletedValues[CMDQUEUE_COPY    ] >= fenceValues[CMDQUEUE_COPY    ]) || (AdvanceCompletion(CMDQUEUE_COPY)     >= fenceValues[CMDQUEUE_COPY    ])) &
                ((m_LastCompletedValues[CMDQUEUE_GRAPHICS] >= fenceValues[CMDQUEUE_GRAPHICS]) || (AdvanceCompletion(CMDQUEUE_GRAPHICS) >= fenceValues[CMDQUEUE_GRAPHICS]));
        }

        void WaitForFence(const u64 fenceValue, const int id) const;
        void WaitForFence(const u64 (&fenceValues)[CMDQUEUE_NUM]) const;

        inline u64 AdvanceCompletion(const int id) const
        {
            // Check current completed fence
            u64 currentCompletedValue = m_pFences[id]->GetCompletedValue();

            if (m_LastCompletedValues[id] < currentCompletedValue)
            {
                DX12_LOG("Completed fence value(s): %d to %d", m_LastCompletedValues[id] + 1, currentCompletedValue);
            }

#ifdef DX12_IN_ORDER_TERMINATION
            DX12_ASSERT(m_LastCompletedValues[id] <= currentCompletedValue, "Getting new fence value which is older than the last!");
            // We do not allow smaller fences being submitted, and fences always complete in-order, no max() neccessary
            m_LastCompletedValues[id] = currentCompletedValue;
#else
            // CLs may terminate in any order. Is it higher than last known completed fence? If so, update it!
            MaxFenceValue(m_LastCompletedValues[id], currentCompletedValue);
#endif // !NDEBUG

            return currentCompletedValue;
        }

        inline void AdvanceCompletion() const
        {
            AdvanceCompletion(CMDQUEUE_COPY);
            AdvanceCompletion(CMDQUEUE_GRAPHICS);
        }

        inline u64 GetLastCompletedFenceValue(const int id) const
        {
            return m_LastCompletedValues[id];
        }

        inline void GetLastCompletedFenceValues(u64 (&fenceValues)[CMDQUEUE_NUM]) const
        {
            fenceValues[CMDQUEUE_COPY    ] = m_LastCompletedValues[CMDQUEUE_COPY    ];
            fenceValues[CMDQUEUE_GRAPHICS] = m_LastCompletedValues[CMDQUEUE_GRAPHICS];
        }

    private:
        Device* m_pDevice;
        ID3D12Fence* m_pFences[CMDQUEUE_NUM];
        HANDLE m_FenceEvents[CMDQUEUE_NUM];

        // Maximum fence-value of all command-lists currently in flight (allocated, running or free)
        std::atomic<u64> m_CurrentValues[CMDQUEUE_NUM];
        // Maximum fence-value of all command-lists passed to the driver (running only)
        std::atomic<u64> m_SubmittedValues[CMDQUEUE_NUM]; // passed to async
        std::atomic<u64> m_SignalledValues[CMDQUEUE_NUM]; // processed by async

        // Maximum fence-value of all command-lists executed by the driver (free only)
        mutable std::atomic<u64> m_LastCompletedValues[CMDQUEUE_NUM];
    };
}
