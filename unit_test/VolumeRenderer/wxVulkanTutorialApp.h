#pragma once

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <wx/wxprec.h>

class wxVulkanTutorialApp :
    public wxApp
{
public:
    wxVulkanTutorialApp();
    virtual ~wxVulkanTutorialApp();
    virtual bool OnInit() override;
};

