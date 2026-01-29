/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#include "InsetSMR.h"
#include "RadarScreen.h"
#include "SectorFileLoad.h"
#include "Version.h"

#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <Windows.h>

namespace InsetSMRNS
{
    InsetSMR::InsetSMR() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE), radarScreen(nullptr)
    {
//        LogEvent("InsetSMR constructed");
        // Report Initialised
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");

        // Load sector file
        sectorFileLoader = new SectorFileLoadNS::SectorFileLoad(this);
        sectorFileLoader->LoadSectorFile();
    }

    void InsetSMR::LogEvent(const std::string &message) {
        std::lock_guard<std::mutex> lock(LogMutex);
        char tempPath[MAX_PATH];
        if (GetTempPathA(MAX_PATH, tempPath) == 0) {
            return; // can't get temp path
        }
        std::string filePath = std::string(tempPath) + "InsetSMR.log";

        std::ofstream ofs(filePath, std::ios::app);
        if (!ofs.is_open()) return;

        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm;
        localtime_s(&tm, &t);

        ofs << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " << message << std::endl;
    }
    InsetSMR::~InsetSMR()
    {
//        LogEvent("InsetSMR destructing");
        // Don't delete radarScreen: EuroScope owns RadarScreen instances created in OnRadarScreenCreated.
    }

    void InsetSMR::DisplayMessage(const std::string &message, const std::string &sender)
    {
        DisplayUserMessage(
            PLUGIN_NAME,
            sender.c_str(),
            message.c_str(),
            true,   // Show Handler
            false,  // Show Unread
            false,  // Show Unread Even If Busy
            false,  // Start Flashing
            false   // Need Confirmation
        );
    }

    void InsetSMR::OnRadarTargetPositionUpdate (EuroScopePlugIn::CRadarTarget RadarTarget) {
        if (!RadarTarget.IsValid()) {
            return; // Invalid radar target
        }
        const char* cs = RadarTarget.GetCallsign();
        std::string callsign = cs ? cs : std::string("<unknown>");
//        LogEvent(std::string("OnRadarTargetPositionUpdate for ") + callsign);

        // Build snapshot from CRadarTarget and store/update it (thread-safe)
        RadarTargetSnapshot s;
        s.valid = true;
        s.callsign = callsign;
        s.lon = RadarTarget.GetPosition().GetPosition().m_Longitude;
        s.lat = RadarTarget.GetPosition().GetPosition().m_Latitude;

        {
            std::lock_guard<std::mutex> lock(ActiveRadarTargetsMutex);
            auto it = std::find_if(ActiveRadarTargets.begin(), ActiveRadarTargets.end(),
                                   [&s](const RadarTargetSnapshot &r) { return r.callsign == s.callsign; });
            if (it != ActiveRadarTargets.end()) {
                *it = s;
            } else {
                ActiveRadarTargets.push_back(std::move(s));
            }
        }
    }

    void InsetSMR::OnFlightPlanDisconnect (EuroScopePlugIn::CFlightPlan FlightPlan) {
        if (!FlightPlan.IsValid()) {
            return; // Invalid flight plan
        }

        const char* cs = FlightPlan.GetCallsign();
        std::string callsign = cs ? cs : std::string("<unknown>");
//        LogEvent(std::string("OnFlightPlanDisconnect for ") + callsign);

        // Remove snapshot(s) matching callsign (thread-safe)
        {
            std::lock_guard<std::mutex> lock(ActiveRadarTargetsMutex);
            ActiveRadarTargets.erase(
                std::remove_if(ActiveRadarTargets.begin(), ActiveRadarTargets.end(),
                               [&callsign](const RadarTargetSnapshot& rt) {
                                   return rt.callsign == callsign;
                               }),
                ActiveRadarTargets.end()
            );
        }
    }

    EuroScopePlugIn::CRadarScreen * InsetSMR::OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) {
        DisplayMessage((std::string("On ") + sDisplayName).c_str(), "Activation");
//        LogEvent(std::string("OnRadarScreenCreated: ") + sDisplayName);
        // Create a new RadarScreen for EuroScope to own.
        return new RadarScreenNS::RadarScreen(this);
    }

    bool InsetSMR::OnCompileCommand ( const char * sCommandLine ) {
        // Handle custom commands here
        std::string UpperCommand = _strupr(_strdup(sCommandLine));
        if (UpperCommand.find(".INSETSMR") != std::string::npos) {
            // Handle .INSETSMR AIRPORT <ICAO>
            if (UpperCommand.find(".INSETSMR AIRPORT ") != std::string::npos) {
                std::string AirportRequested = std::string(UpperCommand).substr(18); // Length of ".INSETSMR AIRPORT "
                if (AirportRequested.length() != 4) {
                    DisplayMessage("Please ensure the requested airport is a 4-letter ICAO code", "Unable to change InsetSMR airport");
                    return true; // Command handled
                }

                // TODO: Implement changing the inset airport
                DisplayMessage(("Unable to change airport to " + AirportRequested).c_str(), "NOT SUPPORTED YET");
                return true; // Command handled
            }

            // Handle .INSETSMR HIDE
            else if (UpperCommand.find(".INSETSMR HIDE") != std::string::npos) {
                DisplayMessage("Hiding InsetSMR is not yet supported", "NOT SUPPORTED YET");
                return true; // Command handled
            }

            // Handle .INSETSMR SHOW
            else if (UpperCommand.find(".INSETSMR SHOW") != std::string::npos) {
                DisplayMessage("Showing InsetSMR is not yet supported", "NOT SUPPORTED YET");
                return true; // Command handled
            }
          
            // Unknown .INSETSMR command
            DisplayMessage("Unknown InsetSMR command. Supported commands are: \".InsetSMR Airport <ICAO>\", \".InsetSMR Hide\", and \".InsetSMR Show\"",
                           "InsetSMR Command Error");

            return true; // Command handled
        }
        return false; // Command not handled
    }

    std::vector<InsetSMR::RadarTargetSnapshot> InsetSMR::getActiveRadarTargetSnapshots() {
        std::lock_guard<std::mutex> lock(ActiveRadarTargetsMutex);
        // ActiveRadarTargets already stores POD snapshots. Return a copy under lock.
        std::vector<RadarTargetSnapshot> snapshots;
        snapshots.reserve(ActiveRadarTargets.size());
        for (const auto &rt : ActiveRadarTargets) {
            snapshots.push_back(rt);
        }
        return snapshots;
    }

    void InsetSMR::SelectAircraftFromFlightPlan(const EuroScopePlugIn::CFlightPlan FlightPlan) {
        EuroScopePlugIn::CPlugIn::SetASELAircraft (FlightPlan);    // NOTE: Using a RadarTarget seems not to work

    }
}