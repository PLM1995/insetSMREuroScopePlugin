/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#pragma once
#include "ViewData.h"
#include "RadarScreen.h"
#include "InsetSMR.h"
#include "SectorFileLoad.h"
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <fstream>
#include <filesystem>
#include "json.hpp"
using json = nlohmann::json;

namespace ViewDataNS
{
    ViewData::ViewData(InsetSMRNS::InsetSMR* pluginInstance) : plugin(pluginInstance) {}

    ViewData::View ViewData::getActiveView() {
        try {
            // if (this) {
            //     try { LogEvent("getActiveView: entry"); } catch(...) {}
            // }

            // Copy the active view under lock, but avoid performing filesystem I/O
            // (LogEvent) while holding the ActiveViewMutex to prevent IO-related
            // exceptions or blocking from propagating into this function.
            View snapshot;
            try {
                std::lock_guard<std::mutex> lock(ActiveViewMutex);
                snapshot = activeView; // <-- narrow try/catch around the copy
            } catch (const std::exception &ex) {
                // Use OutputDebugStringA for diagnostics because filesystem logging
                // may be the cause of failures; OutputDebugStringA is low-overhead
                // and does not rely on disk I/O.
                try {
                    std::string msg = "getActiveView: exception during copy: ";
                    msg += ex.what();
                    msg += "\n";
                    OutputDebugStringA(msg.c_str());
                } catch(...) {}
                return View();
            } catch (...) {
                try { OutputDebugStringA("getActiveView: unknown exception during copy\n"); } catch(...) {}
                return View();
            }

            try {
                if(plugin) plugin->LogEvent(std::string("getActiveView: copying activeAirport.ICAO=") + snapshot.ICAO);
//                LogEvent(std::string("getActiveView: copy done, geoNames=") + std::to_string(snapshot.RelevantGeoNames.size()));
            } catch(...) {
                // LogEvent is already hardened, but swallow any logging errors here too.
            }

            return snapshot;
        } catch (const std::exception &ex) {
            try { if(plugin) plugin->LogEvent(std::string("getActiveAirport exception: ") + ex.what()); } catch(...) {}
            return View();
        } catch (...) {
            try { if(plugin) plugin->LogEvent("getActiveAirport unknown exception"); } catch(...) {}
            return View();
        }
    }

    bool ViewData::setActiveView(std::string ICAO, VIEWMODE viewMode, std::string viewRunway, RadarScreenNS::RadarScreen* radarScreen, SectorFileLoadNS::SectorFileLoad* sectorFileLoader) {
        viewRunway = _strupr(_strdup(viewRunway.c_str()));
        if(plugin) plugin->LogEvent("setActiveView function called for " + ICAO + " at runway " + viewRunway);

        // Look up needed Geo and Region names based on ICAO
        // Load config file
        std::filesystem::path ConfigFilePath = "UK/Data/Plugin/InsetSMR/Config.json";
        std::ifstream configFileStream(ConfigFilePath);
        if (std::filesystem::exists(ConfigFilePath) == false) {
            // File not found
            if(plugin) plugin->DisplayMessage(ConfigFilePath.string().c_str(), "Config file not found at");
            return false;
        }
        else {
//            DisplayMessage(ConfigFilePath.string().c_str(), "Loading config file from");
        }
        if (!configFileStream.is_open()) {
            if(plugin) plugin->DisplayMessage("Failed to open config file", "Error");
            return false;
        }

        // Parse the config file into a JSON Array
        json config;
        try {
            configFileStream >> config;
        } catch (const std::exception &ex) {
            if(plugin) plugin->LogEvent(std::string("Failed to parse Config.json: ") + ex.what());
            return false;
        }

        // Read data
        std::vector<std::string> RelevantGeoNames;
        std::vector<std::string> RelevantRegionNames;
        std::vector<std::string> RelevantExtraLabelsNames;
        std::string ExtraLabelsColour;
        ViewCoordinates viewCoordinates{}; // zero-init to avoid uninitialized values
        bool AirportInJSON = false;
        bool CoordinatesFound = false;
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
                // Extract EXTRA_LABELS and EXTRA_LABEL_COLOUR
                if (config_info.contains("EXTRA_LABELS")) {
                    for (const auto& labelFamily : config_info["EXTRA_LABELS"]) {
                        RelevantExtraLabelsNames.push_back(labelFamily);
                    }
                }
                if (config_info.contains("EXTRA_LABEL_COLOUR")) {
                    ExtraLabelsColour = config_info["EXTRA_LABEL_COLOUR"];
                }
                // Extract Airport SMR View Coordinates
                if (config_info.contains("SMR_COORDs") && viewMode == VIEWMODE::AIRPORT) {
                    const auto& coords = config_info["SMR_COORDs"];
                    if (coords.contains("minLon")) {
                        viewCoordinates.minViewLon = coords["minLon"].get<double>();
                    }
                    if (coords.contains("maxLon")) {
                        viewCoordinates.maxViewLon = coords["maxLon"].get<double>();
                    }
                    if (coords.contains("minLat")) {
                        viewCoordinates.minViewLat = coords["minLat"].get<double>();
                    }
                    if (coords.contains("maxLat")) {
                        viewCoordinates.maxViewLat = coords["maxLat"].get<double>();
                    }
                    CoordinatesFound = true;
                }
                // Extract Runway holding area Coordinates
                if (config_info.contains("HOLDING_AREA_COORDS") && viewMode == VIEWMODE::HOLDINGAREA) {

                    if(plugin) plugin->LogEvent("Looking at HOLDING AREA COORDS in JSON");

                    const auto& runway = config_info["HOLDING_AREA_COORDS"];
                    if (runway.contains(viewRunway)) {
                        const auto& coords = runway[viewRunway];
                        if (coords.contains("minLon")) {
                            viewCoordinates.minViewLon = coords["minLon"].get<double>();
                        }
                        if (coords.contains("maxLon")) {
                            viewCoordinates.maxViewLon = coords["maxLon"].get<double>();
                        }
                        if (coords.contains("minLat")) {
                            viewCoordinates.minViewLat = coords["minLat"].get<double>();
                        }
                        if (coords.contains("maxLat")) {
                            viewCoordinates.maxViewLat = coords["maxLat"].get<double>();
                        }
                        CoordinatesFound = true;
                    }
                }
            }
        }

        // Failed to find data to use
        if (!AirportInJSON || !CoordinatesFound) {
            return false;
        }

        if(plugin) plugin->LogEvent("About to update the active view for: " + ICAO);

        // Update the active airport (hold the lock only for the assignment)
        {
            std::lock_guard<std::mutex> lock(ActiveViewMutex);
            activeView = { ICAO, RelevantGeoNames, RelevantRegionNames, RelevantExtraLabelsNames, ExtraLabelsColour, viewCoordinates };
        }

        // Re-load the sector data (as the sector file loading is airport specific).
        // IMPORTANT: do not hold ActiveAirportMutex while calling into loader
        // because loader may call back into plugin and attempt to acquire the
        // same mutex (causing deadlock) or perform I/O that could block.
        try {
            if(plugin) plugin->LogEvent("Calling SectorFileLoad::LoadSectorFile");
            if (sectorFileLoader) sectorFileLoader->LoadSectorFile();
        } catch (const std::exception &ex) {
            if(plugin) plugin->LogEvent(std::string("Exception loading sector file: ") + ex.what());
            // non-fatal: continue without crashing the plugin
        }

        // Obtain a snapshot of active view to read SMR view coordinates safely
        viewCoordinates = getActiveView().viewCoordinates;
        // Set Radar View Inset Area appropriately
        if (radarScreen) radarScreen->SetInsetViewArea(viewCoordinates);

        // Persist the state (save these settings to asr)
        if (radarScreen) radarScreen->OnAsrContentToBeSaved();

        return true; // Successfully set active view
    }
}