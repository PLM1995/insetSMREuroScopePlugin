/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#pragma once

#include <string>
#include <vector>
#include <mutex>

// Forward declarations to avoid circular includes
namespace InsetSMRNS { class InsetSMR; }
namespace RadarScreenNS { class RadarScreen; }
namespace SectorFileLoadNS { class SectorFileLoad; }

namespace ViewDataNS {
    class ViewData
    {
    public:
        ViewData(InsetSMRNS::InsetSMR* pluginInstance);

        struct ViewCoordinates {
            double minViewLon;
            double minViewLat;
            double maxViewLon;
            double maxViewLat;
        };

        enum VIEWMODE {
            AIRPORT,
            RUNWAY,
            HOLDINGAREA
        };

        struct View {
            std::string ICAO;
            std::vector<std::string> RelevantGeoNames;
            std::vector<std::string> RelevantRegionNames;
            std::vector<std::string> RelevantExtraLabelsNames;
            std::string ExtraLabelsColour;
            ViewCoordinates viewCoordinates;
            enum VIEWMODE activeViewMode;
            std::string activeRunway;
        };

        View getActiveView();
        bool setActiveView(std::string ICAO, VIEWMODE viewMode, std::string viewRunway);

    private:
        View activeView = {};
        std::mutex ActiveViewMutex;

        // Pointer back to plugin instance (owner)
        InsetSMRNS::InsetSMR* plugin = nullptr;
    };
}
