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
#include <filesystem>
#include "json.hpp"
using json = nlohmann::json;
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

            // Initialise Active Airport (default invalid)
            setActiveAirport("ZZZZ"); // TODO: Consider best default.
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

        setActiveAirport("EGPF");

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

                // Change the inset airport
                if (!setActiveAirport(AirportRequested)) {
                    DisplayMessage(AirportRequested.c_str(), "Error setting airport to");
                } else {
                    DisplayMessage(AirportRequested.c_str(), "Set active airport to");
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

            // Hangle .INSETSMR SIZE <SCALE>
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
          
            // Unknown .INSETSMR command
            DisplayMessage("Unknown InsetSMR command. Supported commands are: \".InsetSMR Airport <ICAO>\", \".InsetSMR Size <integar>\", \".InsetSMR Hide\", and \".InsetSMR Show\"",
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
        
    InsetSMR::Airport InsetSMR::getActiveAirport() {
        try {
            // if (this) {
            //     try { LogEvent("getActiveAirport: entry"); } catch(...) {}
            // }

            // Copy the active airport under lock, but avoid performing filesystem I/O
            // (LogEvent) while holding the ActiveAirportMutex to prevent IO-related
            // exceptions or blocking from propagating into this function.
            Airport snapshot;
            try {
                std::lock_guard<std::mutex> lock(ActiveAirportMutex);
                snapshot = activeAirport; // <-- narrow try/catch around the copy
            } catch (const std::exception &ex) {
                // Use OutputDebugStringA for diagnostics because filesystem logging
                // may be the cause of failures; OutputDebugStringA is low-overhead
                // and does not rely on disk I/O.
                try {
                    std::string msg = "getActiveAirport: exception during copy: ";
                    msg += ex.what();
                    msg += "\n";
                    OutputDebugStringA(msg.c_str());
                } catch(...) {}
                return Airport();
            } catch (...) {
                try { OutputDebugStringA("getActiveAirport: unknown exception during copy\n"); } catch(...) {}
                return Airport();
            }

            try {
                LogEvent(std::string("getActiveAirport: copying activeAirport.ICAO=") + snapshot.ICAO);
//                LogEvent(std::string("getActiveAirport: copy done, geoNames=") + std::to_string(snapshot.RelevantGeoNames.size()));
            } catch(...) {
                // LogEvent is already hardened, but swallow any logging errors here too.
            }

            return snapshot;
        } catch (const std::exception &ex) {
            try { LogEvent(std::string("getActiveAirport exception: ") + ex.what()); } catch(...) {}
            return Airport();
        } catch (...) {
            try { LogEvent("getActiveAirport unknown exception"); } catch(...) {}
            return Airport();
        }
    }

    bool InsetSMR::setActiveAirport(std::string ICAO) {
//        LogEvent("setActiveAiport function called for " + ICAO);
        // Check if this is already the active airport
        if (getActiveAirport().ICAO == ICAO) {
            DisplayMessage("Airport already set as active", "Unable");
            return false;
        }

        // TODO: Move this JSON Parsing block somewhere more sensible.
        // Look up needed Geo and Region names based on ICAO
        // Load config file
        std::filesystem::path ConfigFilePath = "UK/Data/Plugin/InsetSMR/Config.json";
        std::ifstream configFileStream(ConfigFilePath);
        if (std::filesystem::exists(ConfigFilePath) == false) {
            // File not found
            DisplayMessage(ConfigFilePath.string().c_str(), "Config file not found at");
            return false;
        }
        else {
//            DisplayMessage(ConfigFilePath.string().c_str(), "Loading config file from");
        }
        if (!configFileStream.is_open()) {
            DisplayMessage("Failed to open config file", "Error");
            return false;
        }

        // Parse the config file into a JSON Array
        json config;
        try {
            configFileStream >> config;
        } catch (const std::exception &ex) {
            LogEvent(std::string("Failed to parse Config.json: ") + ex.what());
            return false;
        }

        // Read data
        std::vector<std::string> RelevantGeoNames;
        std::vector<std::string> RelevantRegionNames;
        ViewCoordinates SMRViewCoordinates{}; // zero-init to avoid uninitialized values
        bool AirportInJSON = false;
        for (const auto& airport : config) {
            // Only load the needed airport data
            if (!airport.contains(ICAO)) {
                continue;
            } else {
                AirportInJSON = true;
            }
            // Load the JSON data into the variables
            for (auto& [config_icao, config_info] : airport.items()) {
                // Extract SMR_GEOs
                if (config_info.contains("SMR_GEOs")) {
                    for (const auto& geo : config_info["SMR_GEOs"]) {
                        RelevantGeoNames.push_back(geo);
                    }
                }
                // Extract SMR_REGIONs
                if (config_info.contains("SMR_REGIONs")) {
                    for (const auto& region : config_info["SMR_REGIONs"]) {
                        RelevantRegionNames.push_back(region);
                    }
                }
                // Extract SMR View
                if (config_info.contains("SMR_COORDs")) {
                    const auto& coords = config_info["SMR_COORDs"];
                    if (coords.contains("minLon")) {
                        SMRViewCoordinates.minViewLon = coords["minLon"].get<double>();
                    }
                    if (coords.contains("maxLon")) {
                        SMRViewCoordinates.maxViewLon = coords["maxLon"].get<double>();
                    }
                    if (coords.contains("minLat")) {
                        SMRViewCoordinates.minViewLat = coords["minLat"].get<double>();
                    }
                    if (coords.contains("maxLat")) {
                        SMRViewCoordinates.maxViewLat = coords["maxLat"].get<double>();
                    }
                }
            }
        }

        // Failed to find data to use
        if (!AirportInJSON) {
            return false;
        }

//        LogEvent("about to update the active airport to: " + ICAO);

        // Update the active airport (hold the lock only for the assignment)
        {
            std::lock_guard<std::mutex> lock(ActiveAirportMutex);
            activeAirport = { ICAO, RelevantGeoNames, RelevantRegionNames, SMRViewCoordinates };
        }

        // Re-load the sector data (as the sector file loading is airport specific).
        // IMPORTANT: do not hold ActiveAirportMutex while calling into loader
        // because loader may call back into plugin and attempt to acquire the
        // same mutex (causing deadlock) or perform I/O that could block.
        try {
            if (sectorFileLoader) {
                LogEvent("Calling SectorFileLoad::LoadSectorFile");
                sectorFileLoader->LoadSectorFile();
//                LogEvent("Returned from SectorFileLoad::LoadSectorFile");
            }
        } catch (const std::exception &ex) {
            LogEvent(std::string("Exception loading sector file: ") + ex.what());
            // non-fatal: continue without crashing the plugin
        }

//        LogEvent("activeAirport object has been updated");

        // Set Radar View Inset Area appropriately (if radar screen exists)
        // Obtain a snapshot of active airport to read SMR view coordinates safely
        ViewCoordinates viewCoordinates = getActiveAirport().SMRViewCoordinates;
//        LogEvent("viewCoordinates set from active airport");
        if (radarScreen) {
            radarScreen->SetInsetViewArea(viewCoordinates);
        }
//        LogEvent("radarScreen should now be updated");
        return true; // Successfully set active airport
    }
}