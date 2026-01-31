/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#pragma once
#include "ViewData.h"
#include "SectorFileLoad.h"

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

        // ViewCoordinates is defined in ViewData.h

        struct View {
            std::string ICAO;
            std::vector<std::string> RelevantGeoNames;
            std::vector<std::string> RelevantRegionNames;
            ViewCoordinates viewCoordinates;
            enum VIEWMODE activeViewMode;
        };

        std::vector<RadarTargetSnapshot> getActiveRadarTargetSnapshots();

        void SelectAircraftFromFlightPlan(const EuroScopePlugIn::CFlightPlan FlightPlan);
        
        View getActiveView();

        bool setActiveView(std::string ICAO, enum VIEWMODE viewmode, std::string viewrunway = "");

    private:
        RadarScreenNS::RadarScreen* radarScreen = nullptr;
        SectorFileLoadNS::SectorFileLoad* sectorFileLoader = nullptr;
        std::mutex ActiveRadarTargetsMutex;
        std::mutex LogMutex;
        std::vector<RadarTargetSnapshot> ActiveRadarTargets = {};

        View activeView = {};
        std::mutex ActiveViewMutex;
    };
}
