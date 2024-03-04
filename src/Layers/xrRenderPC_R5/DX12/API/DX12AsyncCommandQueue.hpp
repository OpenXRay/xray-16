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

#include "DX12CommandListFence.hpp"

struct IDXGISwapChain3;

namespace DX12
{
class CommandListPool;
class AsyncCommandQueue
{
public:
    AsyncCommandQueue();
    ~AsyncCommandQueue();

    static bool IsSynchronous();

    bool Init(CommandListPool* pCommandListPool);
    void Flush(UINT64 lowerBoundFenceValue = ~0ULL);
    void SignalStop()
    {
        m_bStopRequested = true;
    }

    // Equates to the number of pending Present() calls
    int GetQueuedFramesCount() const { return m_QueuedFramesCounter; }

    void ResetCommandList(CommandList* pCommandList);
    void ExecuteCommandLists(UINT NumCommandLists, ID3D12CommandList* const* ppCommandLists);
    void Signal(ID3D12Fence* pFence, const UINT64 Value);
    void Wait(ID3D12Fence* pFence, const UINT64 Value);
    void Wait(ID3D12Fence** pFences, const UINT64 (&Values)[CMDQUEUE_NUM]);

private:
    enum eTaskType
    {
        eTT_ExecuteCommandList,
        eTT_ResetCommandList,
        eTT_SignalFence,
        eTT_WaitForFence,
        eTT_WaitForFences
    };

    struct STaskArgs
    {
        CommandListPool* pCommandListPool;
        volatile int* QueueFramesCounter;
    };

    struct SExecuteCommandlist
    {
        ID3D12CommandList* pCommandList;

        void Process(const STaskArgs& args);
    };

    struct SResetCommandlist
    {
        CommandList* pCommandList;

        void Process(const STaskArgs& args);
    };

    struct SSignalFence
    {
        ID3D12Fence* pFence;
        UINT64 FenceValue;

        void Process(const STaskArgs& args);
    };

    struct SWaitForFence
    {
        ID3D12Fence* pFence;
        UINT64 FenceValue;

        void Process(const STaskArgs& args);
    };

    struct SWaitForFences
    {
        ID3D12Fence** pFences;
        UINT64 FenceValues[CMDQUEUE_NUM];

        void Process(const STaskArgs& args);
    };

    struct SSubmissionTask
    {
        eTaskType type;

        union
        {
            SSignalFence SignalFence;
            SWaitForFence WaitForFence;
            SWaitForFences WaitForFences;
            SExecuteCommandlist ExecuteCommandList;
            SResetCommandlist ResetCommandList;
        } Data;

        template <typename TaskType>
        void Process(const STaskArgs& args)
        {
            TaskType* pTask = reinterpret_cast<TaskType*>(&Data);
            pTask->Process(args);
        }
    };

    template <typename TaskType>
    void AddTask(SSubmissionTask& task)
    {
        if (IsSynchronous())
        {           
            m_TaskQueue.enqueue(task);
           
            // wake up kernel semaphore if we have waiter
            if (InterlockedIncrement((volatile LONG*)&m_QueuedTasksCounter) <= 0)
            {
                ReleaseSemaphore((HANDLE)m_Semaphore, 1, NULL);
            }
        }
        else
        {
            Flush();
            STaskArgs taskArgs = {m_pCmdListPool, &m_QueuedFramesCounter};
            task.Process<TaskType>(taskArgs);
        }
    }

    void ThreadEntry();

    volatile int m_QueuedFramesCounter;
    volatile int m_QueuedTasksCounter;
    volatile bool m_bStopRequested;

    CommandListPool* m_pCmdListPool;
    ConcQueue<UnboundMPSC, SSubmissionTask> m_TaskQueue;
    void *m_Semaphore;
};
};
