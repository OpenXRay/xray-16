#pragma once
#include "CustomDetector.h"

class CUIArtefactDetectorSimple;

class CSimpleDetector : public CCustomDetector
{
    typedef CCustomDetector inherited;

public:
    CSimpleDetector();

protected:
    void Scan() override;
    void CreateUI() override;
    CUIArtefactDetectorSimple& ui() const;
};
