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
        args.pCommandListPool->GetD3D12CommandQueue()->ExecuteCommandLists(1, &pCommandList);
    }

    void AsyncCommandQueue::SResetCommandlist::Process(const STaskArgs& args)
    {
        pCommandList->Reset();
    }

    void AsyncCommandQueue::SSignalFence::Process(const STaskArgs& args)
    {
        args.pCommandListPool->GetD3D12CommandQueue()->Signal(pFence, FenceValue);
        args.pCommandListPool->SetSignalledFenceValue(FenceValue);
    }

    void AsyncCommandQueue::SWaitForFence::Process(const STaskArgs& args)
    {
        args.pCommandListPool->GetD3D12CommandQueue()-> Wait(pFence, FenceValue);
    }

    void AsyncCommandQueue::SWaitForFences::Process(const STaskArgs& args)
    {
        if (FenceValues[CMDQUEUE_COPY    ])
        {
            args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFences[CMDQUEUE_COPY], FenceValues[CMDQUEUE_COPY]);
        }
      
        if (FenceValues[CMDQUEUE_GRAPHICS])
        {
           args.pCommandListPool->GetD3D12CommandQueue()->Wait(pFences[CMDQUEUE_GRAPHICS], FenceValues[CMDQUEUE_GRAPHICS]);
        }
    }

    void AsyncCommandQueue::SPresentBackbuffer::Process(const STaskArgs& args)
    {
        DWORD result = STATUS_WAIT_0;

#ifdef __dxgi1_3_h__
        if (Desc->Flags & DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT)
        {
            // Check if the swapchain is ready to accept another frame
            HANDLE frameLatencyWaitableObject = pSwapChain->GetFrameLatencyWaitableObject();
            //result = WaitForSingleObjectEx(frameLatencyWaitableObject, 500, TRUE);
        }

        if (Desc->Windowed && (Desc->Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING) && !(Flags & DXGI_PRESENT_TEST))
        {
            Flags |= DXGI_PRESENT_ALLOW_TEARING;
            SyncInterval = 0;
        }
#endif

        if (result == WAIT_OBJECT_0)
        {
            *pPresentResult = pSwapChain->Present(SyncInterval, Flags);
        }

        InterlockedDecrement((volatile LONG*)args.QueueFramesCounter);
    }

    AsyncCommandQueue::AsyncCommandQueue()
        : m_pCmdListPool(NULL)
        , m_QueuedFramesCounter(0)
        , m_bStopRequested(false)
        , m_bSleeping(false)
        , m_TaskEvent(INT_MAX, 0)
    {
    }

    AsyncCommandQueue::~AsyncCommandQueue()
    {
        SignalStop();
        Flush();
        m_TaskEvent.Release();

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
        m_bStopRequested = false;
        m_bSleeping = true;

        Threading::SpawnThread(
            [](void* this_ptr) {
                AsyncCommandQueue& self = *static_cast<AsyncCommandQueue*>(this_ptr);
                self.ThreadEntry();
            },
            "DX12 AsyncCommandQueue", 0, this
        );
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

    void AsyncCommandQueue::Present(IDXGISwapChain3* pSwapChain, HRESULT* pPresentResult, UINT SyncInterval,
        UINT Flags, const DXGI_SWAP_CHAIN_DESC& Desc, UINT bufferIndex)
    {
        InterlockedIncrement((volatile LONG*)&m_QueuedFramesCounter);

        SSubmissionTask task;
        ZeroMemory(&task, sizeof(SSubmissionTask));

        task.type = eTT_PresentBackbuffer;
        task.Data.PresentBackbuffer.pSwapChain = pSwapChain;
        task.Data.PresentBackbuffer.pPresentResult = pPresentResult;
        task.Data.PresentBackbuffer.Flags = Flags;
        task.Data.PresentBackbuffer.Desc = &Desc;
        task.Data.PresentBackbuffer.SyncInterval = SyncInterval;

        AddTask<SPresentBackbuffer>(task);

        {
            while (m_QueuedFramesCounter > MAX_FRAMES_GPU_LAG)
            {
                SwitchToThread();
            }
        }
    }

    void AsyncCommandQueue::Flush(UINT64 lowerBoundFenceValue)
    {       
        if (lowerBoundFenceValue != (~0ULL))
        {
            while (lowerBoundFenceValue > m_pCmdListPool->GetSignalledFenceValue())
            {
                SwitchToThread();
            }
        }
        else
        {
            while (!m_bSleeping)
            {
                SwitchToThread();
            }
        }
    }

    void AsyncCommandQueue::FlushNextPresent()
    {
        const int numQueuedFrames = m_QueuedFramesCounter;
        if (numQueuedFrames > 0)
        {
            while (numQueuedFrames == m_QueuedFramesCounter)
            {
                SwitchToThread();
            }
        }
    }

    AsyncCommandQueue::STaskArgs AsyncCommandQueue::GetTaskArg()
    {
        return {m_pCmdListPool, &m_QueuedFramesCounter};
    }

    void AsyncCommandQueue::ThreadEntry()
    {
        SSubmissionTask task;

        while (!m_bStopRequested)
        {
            m_TaskEvent.Acquire();

            while (m_TaskQueue.dequeue(task))
            {
                switch (task.type)
                {
                case eTT_ExecuteCommandList: task.Process<SExecuteCommandlist>(GetTaskArg()); break;
                case eTT_ResetCommandList: task.Process<SResetCommandlist>(GetTaskArg()); break;
                case eTT_SignalFence: task.Process<SSignalFence>(GetTaskArg()); break;
                case eTT_WaitForFence: task.Process<SWaitForFence>(GetTaskArg()); break;
                case eTT_WaitForFences: task.Process<SWaitForFences>(GetTaskArg()); break;
                case eTT_PresentBackbuffer: task.Process<SPresentBackbuffer>(GetTaskArg()); break;
                }              
            };
        }
    }
}
