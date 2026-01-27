/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

 #pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

//Forward Declaration
namespace InsetSMRNS {
    class InsetSMR;
}

namespace RadarScreenNS 
{
    class RadarScreen : public EuroScopePlugIn::CRadarScreen
    {
    public:
        RadarScreen(InsetSMRNS::InsetSMR* pluginInstance);

        virtual void OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button);
        virtual void OnRefresh(HDC hDC, int phase);
        virtual void OnAsrContentToBeClosed() override { /* no-op */ }

    private:
        InsetSMRNS::InsetSMR* plugin;
    };
}
