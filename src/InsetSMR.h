/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#pragma once
// Forward declare ViewData to avoid circular include
namespace ViewDataNS { class ViewData; }
#include "SectorFileLoad.h"
#include "ViewData.h"

// Forward declare RadarScreen namespace/class to avoid circular include
namespace RadarScreenNS { class RadarScreen; }

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>
#include <map>
#include <vector>
#include <mutex>

enum class PushbackDirection {
    Null = 0,
    Straight = 1,
    Left = 2,
    Right = 3
};

namespace InsetSMRNS
{
    class InsetSMR : public EuroScopePlugIn::CPlugIn
    {
    public:
        InsetSMR();
        ~InsetSMR();

        RadarScreenNS::RadarScreen* GetRadarScreen() { return radarScreen; }

        SectorFileLoadNS::SectorFileLoad* GetSectorFileLoader() { return sectorFileLoader; }

        void DisplayMessage(const std::string &message,
                            const std::string &sender = "InsetSMR");

        void LogEvent(const std::string &message);

        virtual void OnRadarTargetPositionUpdate (EuroScopePlugIn::CRadarTarget RadarTarget);

        virtual void OnFlightPlanDisconnect (EuroScopePlugIn::CFlightPlan FlightPlan);

        virtual EuroScopePlugIn::CRadarScreen * OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

        virtual bool OnCompileCommand ( const char * sCommandLine );

        struct RadarTargetSnapshot {
            double lon = 0.0;
            double lat = 0.0;
            std::string callsign;
            std::string acType;
            std::string SID;
            bool valid = false;
        };

        std::vector<RadarTargetSnapshot> getActiveRadarTargetSnapshots();

        ViewDataNS::ViewData::View getActiveView();
        bool RequestSetActiveView(const std::string &ICAO, ViewDataNS::ViewData::VIEWMODE viewMode, const std::string &viewRunway = "");

        void SelectAircraftFromFlightPlan(const EuroScopePlugIn::CFlightPlan FlightPlan);

    private:
        RadarScreenNS::RadarScreen* radarScreen = nullptr;
        SectorFileLoadNS::SectorFileLoad* sectorFileLoader = nullptr;
        ViewDataNS::ViewData* viewData = nullptr;
        std::mutex ActiveRadarTargetsMutex;
        std::mutex LogMutex;
        std::vector<RadarTargetSnapshot> ActiveRadarTargets = {};
    };
}
