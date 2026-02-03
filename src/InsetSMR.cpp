/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#include "InsetSMR.h"
#include "RadarScreen.h"
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
        try {
//        LogEvent("InsetSMR constructed");
            // Report Initialised
            DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");

            // Create sector file loader before setting active airport (loader used by setActiveAirport)
            sectorFileLoader = new SectorFileLoadNS::SectorFileLoad(this);
            viewData = new ViewDataNS::ViewData(this);
        }
        catch (const std::exception &ex) {
            try { LogEvent(std::string("Exception in InsetSMR ctor: ") + ex.what()); } catch(...) {}
            DisplayMessage("InsetSMR failed to initialize (see log)", "InsetSMR");
        }
        catch (...) {
            try { LogEvent("Unknown exception in InsetSMR ctor"); } catch(...) {}
            DisplayMessage("InsetSMR failed to initialize (see log)", "InsetSMR");
        }
    }

    InsetSMR::~InsetSMR()
    {
//        LogEvent("InsetSMR destructing");
        // Don't delete radarScreen: EuroScope owns RadarScreen instances created in OnRadarScreenCreated.
    }

    void InsetSMR::LogEvent(const std::string &message) {
        // Protect LogEvent from throwing due to transient filesystem / sharing errors
        try {
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
        } catch (...) {
            // Swallow any exceptions from logging (filesystem/sharing/IO errors can occur,
            // e.g. OneDrive/antivirus holding the file). Don't let logging break plugin.
        }
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
        s.acType = RadarTarget.GetCorrelatedFlightPlan().GetFlightPlanData().GetAircraftFPType();
        s.SID = RadarTarget.GetCorrelatedFlightPlan().GetFlightPlanData().GetSidName();
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
        radarScreen = new RadarScreenNS::RadarScreen(this);

        // Initialise Default View, expect this to be overridden by .asr settings if they exist
        enum ViewDataNS::ViewData::VIEWMODE defaultVIEWMODE = ViewDataNS::ViewData::HOLDINGAREA;
        std::string defaultRunway = "27L";
        std::string defaultAirport = "EGLL";
        radarScreen->setActiveView(defaultAirport, defaultVIEWMODE, defaultRunway);

        return radarScreen;
    }

    bool InsetSMR::OnCompileCommand ( const char * sCommandLine ) {
        // Handle custom commands here
        std::string UpperCommand = _strupr(_strdup(sCommandLine));
        if (UpperCommand.find(".INSETSMR") != std::string::npos) {
            // Handle .INSETSMR AIRPORT <ICAO>
            if (UpperCommand.find(".INSETSMR AIRPORT ") != std::string::npos) {
                std::string AirportRequested = std::string(UpperCommand).substr(18); // 18 is length of ".INSETSMR AIRPORT "
                if (AirportRequested.length() != 4) {
                    DisplayMessage("Please ensure the requested airport is a 4-letter ICAO code", "Unable to change InsetSMR airport");
                    return true; // Command handled
                }

                // Change the inset view
                enum ViewDataNS::ViewData::VIEWMODE VIEWMODE = ViewDataNS::ViewData::AIRPORT;
                if (!radarScreen->setActiveView(AirportRequested, VIEWMODE)) {
                    DisplayMessage(AirportRequested.c_str(), "Error setting airport to");
                } else {
                    DisplayMessage(AirportRequested.c_str(), "Set active airport to");
                }
                return true; // Command handled
            }

            // Handle .INSETSMR HOLDING <ICAO> <RUNWAY>
            else if (UpperCommand.find(".INSETSMR HOLDING ") != std::string::npos) {
                std::string AirportRunwayRequested = std::string(UpperCommand).substr(18); // 18 is length of ".INSETSMR HOLDING "
                std::string ICAORequested = AirportRunwayRequested.substr(0, 4);
                std::string RunwayRequest = AirportRunwayRequested.substr(5); // 5 if length og "ICAO "

                // Change the inset view
                enum ViewDataNS::ViewData::VIEWMODE VIEWMODE = ViewDataNS::ViewData::HOLDINGAREA;
                if (!radarScreen->setActiveView(ICAORequested, VIEWMODE, RunwayRequest)) {
                    DisplayMessage("not managed to change to holding area view specified", "Error");
                } else {
                    DisplayMessage(ICAORequested.c_str(), "Set active holding area view for");
                }
                return true; // Command handled
            }

            // Handle .INSETSMR HIDE
            else if (UpperCommand.find(".INSETSMR HIDE") != std::string::npos) {
                if (!radarScreen) {
                    DisplayMessage("No radar screen available to hide", "InsetSMR");
                    return true;
                }
                if (radarScreen->SetShowingInsetSMR(false)) {
                    DisplayMessage("InsetSMR, To show again, use \".InsetSMR Show\"", "Hiding");
                } else {
                    DisplayMessage("InsetSMR", "Already hidden");
                }
                return true; // Command handled
            }

            // Handle .INSETSMR SHOW
            else if (UpperCommand.find(".INSETSMR SHOW") != std::string::npos) {
                if (!radarScreen) {
                    DisplayMessage("No radar screen available to show", "InsetSMR");
                    return true;
                }
                if (radarScreen->SetShowingInsetSMR(true)) {
                    DisplayMessage("InsetSMR. To hide again, use \".InsetSMR Hide\"", "Showing");
                } else {
                    DisplayMessage("InsetSMR", "Already showing");
                }
                return true; // Command handled
            }

            // Handle .INSETSMR DATALINE
            else if (UpperCommand.find(".INSETSMR DATALINE") != std::string::npos) {
                radarScreen->ToggleDataLine();
                return true; // Command handled
            }

            // Handle .INSETSMR LABELS
            else if (UpperCommand.find(".INSETSMR LABELS") != std::string::npos) {
                radarScreen->ToggleLabels();
                return true; // Command handled
            }

            // Handle .INSETSMR SIZE <SCALE>
            else if (UpperCommand.find(".INSETSMR SIZE ") != std::string::npos) {
                int ScaleFactor = std::stoi(std::string(UpperCommand).substr(15).c_str()); // 15 is length of ".INSETSMR AIRPORT "
                if (ScaleFactor < 1 || ScaleFactor > 9) {
                    DisplayMessage("Please ensure the size is an integar between 1 and 9", "Unable to change size");
                    return true; // Command handled
                }

                // Change the size
                radarScreen->SetScaleFactor(ScaleFactor);
                DisplayMessage(std::to_string(ScaleFactor), "Set size to");
                return true; // Command handled
            }

            // Handle .INSETSMR RESETPOSITION
            else if (UpperCommand.find(".INSETSMR RESETPOSITION") != std::string::npos) {
                radarScreen->resetTopLeftPosition();
                return true; // Command handled
            }
            
            // Unknown .INSETSMR command
            DisplayMessage("Unknown InsetSMR command. Supported commands are: \".InsetSMR Airport <ICAO>\", \".InsetSMR Holding <ICAO> <RUNWAY>\", \".InsetSMR Size <SCALE>\", \".InsetSMR Dataline\", \".InsetSMR Labels\", \".InsetSMR Hide\", \".InsetSMR Show\", and \".InsetSMR ResetPosition\"",
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

    ViewDataNS::ViewData::View InsetSMR::getActiveView()
    {
        if (viewData) return viewData->getActiveView();
        return ViewDataNS::ViewData::View();
    }

    bool InsetSMR::RequestSetActiveView(const std::string &ICAO, ViewDataNS::ViewData::VIEWMODE viewMode, const std::string &viewRunway)
    {
        if (!viewData) return false;
        try {
            return viewData->setActiveView(ICAO, viewMode, viewRunway);
        } catch (...) {
            try { LogEvent("RequestSetActiveView: exception while setting active view"); } catch(...) {}
            return false;
        }
    }

    void InsetSMR::SelectAircraftFromFlightPlan(const EuroScopePlugIn::CFlightPlan FlightPlan) {
        EuroScopePlugIn::CPlugIn::SetASELAircraft (FlightPlan);    // NOTE: Using a RadarTarget seems not to work
    }
}