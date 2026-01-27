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

namespace InsetSMRNS
{
    InsetSMR::InsetSMR() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE), radarScreen(nullptr)
    {
        // Create and register
        radarScreen = new RadarScreenNS::RadarScreen(this);

        // Report Initialised
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");

        // Load sector file
        sectorFileLoader = new SectorFileLoadNS::SectorFileLoad(this);
        sectorFileLoader->LoadSectorFile();

    }
    InsetSMR::~InsetSMR()
    {
        // Tidy up
        if (radarScreen) {
            delete radarScreen;
            radarScreen = nullptr;
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

    EuroScopePlugIn::CRadarScreen * InsetSMR::OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) {
        DisplayMessage((std::string("On ") + sDisplayName).c_str(), "Activation");
        
        return new RadarScreenNS::RadarScreen(this);
    }

    // Check for disconnected aircraft every second
    void InsetSMR::OnTimer (int Counter) {
        // TODO: Implement
    }
}