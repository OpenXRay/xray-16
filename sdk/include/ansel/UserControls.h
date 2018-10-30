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
// Copyright 2017 NVIDIA Corporation. All rights reserved.
#pragma once
#include <cstdint>
#include <ansel/Defines.h>

namespace ansel
{
    enum UserControlStatus
    {
        kUserControlOk = 0,
        kUserControlIdAlreadyExists,
        kUserControlInvalidId,
        kUserControlInvalidType,
        kUserControlInvalidLabel,
        kUserControlNameTooLong,
        kUserControlInvalidValue,
        kUserControlInvalidLocale,
        kUserControlInvalidCallback
    };

    enum UserControlType
    {
        kUserControlSlider = 0,
        kUserControlBoolean
    };

    // This is a structure that is being passed into the UserControlCallback and used to specify 
    // a control about to be created.
    // It contains id of control that was changed in the Ansel UI or about to be created, it's type and
    // associated value
    // Valid user id is not used by any other control and is an integer starting from 0.
    //
    // The lifetime of value pointer is only limited to the UserControlCallback callback or addUserControl execution
    //
    // In case the type is kUserControlSlider:
    //    value should be a pointer to a floating point value in [0.0, 1.0] inclusive range
    // In case the type is kUserControlBoolean:
    //    value should be a pointer to a boolean value, which is true in case the control is in checked/selected state, otherwise false
    //
    struct UserControlInfo
    {
        // Any ID user wants
        uint32_t userControlId;
        // A type of the user control (slider, boolean)
        UserControlType userControlType;
        // A value - boolean or float, depending on the control type
        // Needs to be casted to the apropriate type and dereferenced 
        const void* value;
        // User defined pointer which is then passed to all the callbacks (nullptr by default)
        void* userPointer;

        UserControlInfo()
        {
            userControlId = 0;
            userControlType = UserControlType::kUserControlBoolean;
            value = nullptr;
            userPointer = nullptr;
        }
    };

    typedef void(*UserControlCallback)(const UserControlInfo& info);

    // This structure describes a control to be added.
    // This function requires a valid UserControlCallback callback, a UserControlInfo
    // (id, control type and default value) and a label (non nullptr and non
    // empty string without '\n', '\r' and '\t') encoded in utf8. The length of the label should
    // not exceed 20 characters not counting the trailing zero.
    struct UserControlDesc
    {
        const char* labelUtf8; 
        UserControlCallback callback;
        UserControlInfo info;

        UserControlDesc()
        {
            labelUtf8 = nullptr;
            callback = nullptr;
        }
    };

    // This function adds a user control defined with the UserControlDesc object
    ANSEL_SDK_API UserControlStatus addUserControl(const UserControlDesc& desc);
    // This function specifies a translation for a control label.
    // This function requires a valid lang (a-la "en-US", "es-ES", etc) and a label (non nullptr and non
    // empty string without '\n', '\r' and '\t') encoded in utf8. The length of the label should
    // not exceed 20 characters not counting the trailing zero
    ANSEL_SDK_API UserControlStatus setUserControlLabelLocalization(uint32_t userControlId, const char* lang, const char* labelUtf8);
    // This function removes a control that was added previously
    ANSEL_SDK_API UserControlStatus removeUserControl(uint32_t userControlId);
    // This function returns the current control value
    ANSEL_SDK_API UserControlStatus getUserControlValue(uint32_t userControlId, void* value);
    // This function sets the current control value
    ANSEL_SDK_API UserControlStatus setUserControlValue(uint32_t userControlId, const void* value);
}
