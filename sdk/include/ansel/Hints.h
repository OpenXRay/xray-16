// This code contains NVIDIA Confidential Information and is disclosed to you
// under a form of NVIDIA software license agreement provided separately to you.
//
// Notice
// NVIDIA Corporation and its licensors retain all intellectual property and
// proprietary rights in and to this software and related documentation and
// any modifications thereto. Any use, reproduction, disclosure, or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA Corporation is strictly prohibited.
//
// ALL NVIDIA DESIGN SPECIFICATIONS, CODE ARE PROVIDED "AS IS.". NVIDIA MAKES
// NO WARRANTIES, EXPRESSED, IMPLIED, STATUTORY, OR OTHERWISE WITH RESPECT TO
// THE MATERIALS, AND EXPRESSLY DISCLAIMS ALL IMPLIED WARRANTIES OF NONINFRINGEMENT,
// MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE.
//
// Information and code furnished is believed to be accurate and reliable.
// However, NVIDIA Corporation assumes no responsibility for the consequences of use of such
// information or for any infringement of patents or other rights of third parties that may
// result from its use. No license is granted by implication or otherwise under any patent
// or patent rights of NVIDIA Corporation. Details are subject to change without notice.
// This code supersedes and replaces all information previously supplied.
// NVIDIA Corporation products are not authorized for use as critical
// components in life support devices or systems without express written approval of
// NVIDIA Corporation.
//
// Copyright 2016 NVIDIA Corporation. All rights reserved.

#pragma once
#include <cstdint>
#include <ansel/Defines.h>

namespace ansel
{
    enum BufferType
    {
        kBufferTypeHDR = 0,
        kBufferTypeDepth,
        kBufferTypeHUDless,
        kBufferTypeFinalColor,
        kBufferTypeCount
    };

    enum HintType
    {
        kHintTypePreBind,
        kHintTypePostBind
    };

    enum HintThreadingBehaviour
    {
        kThreadingBehaviourMatchAutomatic = 0,
        kThreadingBehaviourNoMatching = 0xFFFFFFFFFFFFFFFFull
    };

    // Call this right before setting HDR render target active
    // bufferType is an optional argument specifying what type of buffer is this - 
    // an HDR color buffer, a depth buffer or HUDless buffer. The default option is HDR color buffer.
    // hintType is an optional argument specifying what type of hint is this -
    // it could be called after or before the bind of a buffer that this hint marks.
    // The default option is kHintTypePreBind, which means the hint should be called before 
    // the render target is bound.
    // threadId is an optional argument allowing Ansel to match the thread which calls
    // SetRenderTarget (or analogous function, since this is graphics API dependent)
    // to the thread which called the hint. The default value of kNoMatching
    // means that no such matching is going to happen. The special value of 0 means that
    // Ansel SDK is going to match thread ids automatically. Any other value means a specific thread id
    // known at integration side.
    ANSEL_SDK_API void markBufferBind(BufferType bufferType = kBufferTypeHDR, HintType hintType = kHintTypePreBind, uint64_t threadId = kThreadingBehaviourNoMatching);
    // Call this right after the last draw call into the HDR render target
    // bufferType is an optional argument specifying what type of buffer is this - 
    // an HDR color buffer, a depth buffer or HUDless buffer. The default option is HDR color buffer.
    // threadId is an optional argument allowing Ansel to match the thread which calls
    // SetRenderTarget (or analogous function, since this is graphics API dependent)
    // to the thread which called the hint. The default value of kNoMatching
    // means that no such matching is going to happen. The special value of 0 means that
    // Ansel SDK is going to match thread ids automatically. Any other value means a specific thread id
    // known at integration side.
    ANSEL_SDK_API void markBufferFinished(BufferType bufferType = kBufferTypeHDR, uint64_t threadId = kThreadingBehaviourNoMatching);
}

