#pragma once
#include "StdAfx.h"
#include "xrCore/Animation/Motion.hpp"

class SMotionLW : public CSMotion
{
public:
    virtual void Save(IWriter& writer) override;
    void ParseBoneMotion(LWItemID bone);
};
