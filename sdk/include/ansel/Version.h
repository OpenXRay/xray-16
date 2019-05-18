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

// These are the version numbers used to version the API between Ansel SDK and the driver.
// The driver provides backwards compatibility to all MINOR versions that are equal or lower
// but requires matching MAJOR version. This effectively means that MAJOR version number will *not*
// change because this would mean that a newer driver would stop supporting older Ansel SDKs.
// The driver does however not provide forwards compatibility, i.e. when a new interface or change 
// in behavior is introduced between Ansel SDK and the driver the MINOR version is incremented. This
// means that new driver supporting this new MINOR version is required. To track this relationship 
// we use the MINIMUM_DRIVER_VERSION define. When you increment MINOR version you need to adjust the
// MINIMUM_DRIVER_VERSION as well. While a public driver hasn't been finalized use our internal 
// release tags to identify it (like 378.GA2).
#define ANSEL_SDK_PRODUCT_VERSION_MAJOR 1
#define ANSEL_SDK_PRODUCT_VERSION_MINOR 6
#define ANSEL_SDK_MINIMUM_DRIVER_VERSION "396.GA6"

// The lines below are automatically updated by build agents. Please don't touch.
// The BUILD_NUMBER and COMMIT_HASH are useful in uniquely identifying a build of the Ansel SDK.
// Changes to the customer facing API can be tracked with these since they are automatically 
// updated every time a change is made.
#define ANSEL_SDK_BUILD_NUMBER 490
#define ANSEL_SDK_COMMIT_HASH 0xa7eda3e4

#define ANSEL_SDK_VERSION ( uint64_t(ANSEL_SDK_PRODUCT_VERSION_MAJOR) << 48 | uint64_t(ANSEL_SDK_PRODUCT_VERSION_MINOR) << 32 \
                           | ANSEL_SDK_COMMIT_HASH )
