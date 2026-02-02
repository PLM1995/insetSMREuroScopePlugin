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
#include "ViewData.h"

namespace RadarScreenNS 
{
    class RadarScreen : public EuroScopePlugIn::CRadarScreen
    {
    public:
        RadarScreen(InsetSMRNS::InsetSMR* pluginInstance);

        virtual void OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button);
        virtual void OnMoveScreenObject ( int ObjectType, const char * sObjectId, POINT Pt, RECT Area, bool Released );
        virtual void OnRefresh(HDC hDC, int phase);
        virtual void OnAsrContentToBeClosed() override { /* no-op */ }

        bool SetShowingInsetSMR(bool show);

        void SetInsetViewArea(InsetSMRNS::ViewCoordinates viewArea);

        void SetScaleFactor(int newScaleFactor);

        bool ToggleDataLine();

        bool ToggleLabels();

    private:
        // Initially top-left corner
        // TODO: Allow saving/loading of position to/from the .asr
        POINT insetTopLeftPosition = { 10, 50 };

        InsetSMRNS::InsetSMR* plugin;

        bool isShowingInsetSMR();
        bool showingInsetSMR = true;
        std::mutex showingInsetSMRMutex;

        void ToggleInsetSMRMinimised();
        bool insetSMRMinimised = false;
        std::mutex insetSMRMinimisedMutex;
        bool isInsetSMRMinimised();

        bool showDataLine = true;
        std::mutex showDataLineMutex;
        bool isDataLineShown();

        bool showLabels = true;
        std::mutex showLabelsMutex;
        bool areLabelsShown();

        InsetSMRNS::ViewCoordinates insetViewArea = { -4.453889, 55.861389, -4.416389, 55.880833 }; // Default is EGPF
        InsetSMRNS::ViewCoordinates getInsetViewArea();
        std::mutex insetViewAreaMutex;

        int scaleFactor = 3;
        std::mutex scaleFactorMutex;
        int getScaleFactor();

        // const Elements for drawing (to save runtime creation)
        const HFONT buttonFont = CreateFontA(8, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                             ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                             DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                             "Arial");
        const HFONT titleFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                            ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                            "Arial");
        const HFONT labelFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                            ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                            "Arial");
        const HPEN buttonPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)); // Black color
        const int buttonSize = 10;
        const char* title = "InsetSMR";
    };
}
