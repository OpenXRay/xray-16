#pragma once
#include "CustomDetector.h"
#include "Level.h"

class CUIArtefactDetectorElite;

class CEliteDetector : public CCustomDetector
{
    using inherited = CCustomDetector;

public:
    CEliteDetector();

    void render_item_3d_ui() override;
    bool render_item_3d_ui_query() override;

    virtual LPCSTR ui_xml_tag() const { return "elite"; }

protected:
    void Scan() override;
    void CreateUI() override;
    CUIArtefactDetectorElite& ui() const;
};

class CScientificDetector : public CEliteDetector
{
public:
    pcstr ui_xml_tag() const override { return "scientific"; }
};
