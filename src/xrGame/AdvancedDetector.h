#pragma once
#include "CustomDetector.h"
class CUIArtefactDetectorAdv;

class CAdvancedDetector : public CCustomDetector
{
    typedef CCustomDetector inherited;

public:
    CAdvancedDetector();

    void on_a_hud_attach() override;
    void on_b_hud_detach() override;

protected:
    void Scan() override;
    void CreateUI() override;
    CUIArtefactDetectorAdv& ui() const;
};
