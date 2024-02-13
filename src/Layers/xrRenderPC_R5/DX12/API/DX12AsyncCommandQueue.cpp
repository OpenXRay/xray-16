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

#include "StdAfx.h"
#include "DX12AsyncCommandQueue.hpp"
#include "DX12CommandList.hpp"
#include "DX12Device.hpp"

namespace DX12
{
    void AsyncCommandQueue::SExecuteCommandlist::Process(const STaskArgs& args)
    {
        args.pCommandQueue->ExecuteCommandLists(1, &pCommandList);
    }

    void AsyncCommandQueue::SResetCommandlist::Process(const STaskArgs& args)
    {
        pCommandList->Reset();
    }

    void AsyncCommandQueue::SSignalFence::Process(const STaskArgs& args)
    {
        args.pCommandQueue->Signal(pFence, FenceValue);
    }

    void AsyncCommandQueue::SWaitForFence::Process(const STaskArgs& args)
    {
        args.pCommandQueue->Wait(pFence, FenceValue);
    }

    void AsyncCommandQueue::SWaitForFences::Process(const STaskArgs& args)
    {
        if (FenceValues[CMDQUEUE_COPY    ])
        {
            args.pCommandQueue->Wait(pFences[CMDQUEUE_COPY    ], FenceValues[CMDQUEUE_COPY    ]);
        }
      
        if (FenceValues[CMDQUEUE_GRAPHICS])
        {
            args.pCommandQueue->Wait(pFences[CMDQUEUE_GRAPHICS], FenceValues[CMDQUEUE_GRAPHICS]);
        }
    }

    AsyncCommandQueue::AsyncCommandQueue()
        : m_pCmdListPool(NULL)
        , m_QueuedFramesCounter(0)
        , m_bStopRequested(false)
    {
    }

    AsyncCommandQueue::~AsyncCommandQueue()
    {
        SignalStop();
        Flush();
        m_pCmdListPool = nullptr;
    }

    bool AsyncCommandQueue::IsSynchronous()
    {
        return ps_r2_ls_flags_ext.test(R5FLAGEXT_SUBMISSION_THREAD);
    }

    void AsyncCommandQueue::Init(CommandListPool* pCommandListPool)
    {
        m_pCmdListPool = pCommandListPool;
        m_QueuedFramesCounter = 0;
        m_QueuedTasksCounter = 0;
        m_bStopRequested = false;

        m_thread_task = std::thread([this] { ThreadEntry(); });
        m_thread_task.detach();
    }

    void AsyncCommandQueue::ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists)
    {
        for (int i = 0; i < NumCommandLists; ++i)
        {
            SSubmissionTask task;
            ZeroMemory(&task, sizeof(SSubmissionTask));

            task.type = eTT_ExecuteCommandList;
            task.Data.ExecuteCommandList.pCommandList = ppCommandLists[i];

            AddTask<SExecuteCommandlist>(task);
        }
    }

    void AsyncCommandQueue::ResetCommandList(CommandList* pCommandList)
    {
        SSubmissionTask task;
        ZeroMemory(&task, sizeof(SSubmissionTask));

        task.type = eTT_ResetCommandList;
        task.Data.ResetCommandList.pCommandList = pCommandList;

        AddTask<SResetCommandlist>(task);
    }

    void AsyncCommandQueue::Signal(ID3D12Fence* pFence, const UINT64 FenceValue)
    {
        SSubmissionTask task;
        ZeroMemory(&task, sizeof(SSubmissionTask));

        task.type = eTT_SignalFence;
        task.Data.SignalFence.pFence = pFence;
        task.Data.SignalFence.FenceValue = FenceValue;

        AddTask<SSignalFence>(task);
    }

    void AsyncCommandQueue::Wait(ID3D12Fence* pFence, const UINT64 FenceValue)
    {
        SSubmissionTask task;
        ZeroMemory(&task, sizeof(SSubmissionTask));

        task.type = eTT_WaitForFence;
        task.Data.WaitForFence.pFence = pFence;
        task.Data.WaitForFence.FenceValue = FenceValue;

        AddTask<SWaitForFence>(task);
    }

    void AsyncCommandQueue::Wait(ID3D12Fence** pFences, const UINT64 (&FenceValues)[CMDQUEUE_NUM])
    {
        SSubmissionTask task;
        ZeroMemory(&task, sizeof(SSubmissionTask));

        task.type = eTT_WaitForFences;
        task.Data.WaitForFences.pFences = pFences;
        task.Data.WaitForFences.FenceValues[CMDQUEUE_COPY    ] = FenceValues[CMDQUEUE_COPY    ];
        task.Data.WaitForFences.FenceValues[CMDQUEUE_GRAPHICS] = FenceValues[CMDQUEUE_GRAPHICS];

        AddTask<SWaitForFences>(task);
    }

    void AsyncCommandQueue::Flush(UINT64 lowerBoundFenceValue)
    {       
        if (IsSynchronous())
        {
            std::unique_lock lk(m_mutex_for_flush);
            m_cv_for_flush.wait(lk, [this, lowerBoundFenceValue] {
                return (m_QueuedTasksCounter == 0 && lowerBoundFenceValue == (~0ULL)) ||
                    (m_QueuedTasksCounter == 0 && lowerBoundFenceValue != (~0ULL) &&
                        lowerBoundFenceValue > m_pCmdListPool->GetLastCompletedFenceValue());
            });

            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lk.unlock();
        }
        else
        {
            if (lowerBoundFenceValue != (~0ULL))
            {
                while (m_QueuedTasksCounter > 0)
                {
                    if (lowerBoundFenceValue <= m_pCmdListPool->GetLastCompletedFenceValue())
                    {
                        break;
                    }

                    std::this_thread::sleep_for(std::chrono::seconds(0));
                }
            }
            else
            {
                while (m_QueuedTasksCounter > 0)
                {
                    std::this_thread::sleep_for(std::chrono::seconds(0));
                }
            }
        }
    }

    AsyncCommandQueue::STaskArgs AsyncCommandQueue::GetTaskArg()
    {
        return {m_pCmdListPool->GetD3D12CommandQueue(), &m_QueuedFramesCounter};
    }

    void AsyncCommandQueue::ThreadEntry()
    {
        SSubmissionTask task;

        while (true)
        {
            // Wait until main() sends data
            std::unique_lock lk(m_mutex_task);
            m_cv_task.wait(lk, [this] { return m_TaskQueue.is_dequeue() || m_bStopRequested; });

            while (m_TaskQueue.dequeue(task))
            {
                switch (task.type)
                {
                case eTT_ExecuteCommandList: task.Process<SExecuteCommandlist>(GetTaskArg()); break;
                case eTT_ResetCommandList: task.Process<SResetCommandlist>(GetTaskArg()); break;
                case eTT_SignalFence: task.Process<SSignalFence>(GetTaskArg()); break;
                case eTT_WaitForFence: task.Process<SWaitForFence>(GetTaskArg()); break;
                case eTT_WaitForFences: task.Process<SWaitForFences>(GetTaskArg()); break;
                }
                
                InterlockedDecrement((volatile LONG*)&m_QueuedTasksCounter);
            };

            if (m_bStopRequested)
                break;

            std::lock_guard lk_2(m_mutex_for_flush);
            m_cv_for_flush.notify_one();

            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lk.unlock();
        }
    }
}
