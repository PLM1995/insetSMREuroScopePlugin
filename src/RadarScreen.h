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

#include <mutex>

//Forward Declaration
namespace InsetSMRNS {
    class InsetSMR;
}
#include "ViewCoordinates.h"

namespace RadarScreenNS 
{
    class RadarScreen : public EuroScopePlugIn::CRadarScreen
    {
    public:
        RadarScreen(InsetSMRNS::InsetSMR* pluginInstance);

        virtual void OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button);
        virtual void OnRefresh(HDC hDC, int phase);
        virtual void OnAsrContentToBeClosed() override { /* no-op */ }

        bool SetShowingInsetSMR(bool show);

        void SetInsetViewArea(InsetSMRNS::ViewCoordinates viewArea);

    private:
        InsetSMRNS::InsetSMR* plugin;

        bool isShowingInsetSMR();
        bool showingInsetSMR = true;
        std::mutex showingInsetSMRMutex;

        void ToggleInsetSMRMinimised();
        bool insetSMRMinimised = false;
        std::mutex insetSMRMinimisedMutex;
        bool isInsetSMRMinimised();

        InsetSMRNS::ViewCoordinates insetViewArea = { -4.453889, 55.861389, -4.416389, 55.880833 }; // Default is EGPF
        InsetSMRNS::ViewCoordinates getInsetViewArea();
        std::mutex insetViewAreaMutex;
    };
}
