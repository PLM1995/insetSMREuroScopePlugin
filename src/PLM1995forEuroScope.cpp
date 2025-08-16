#include "PLM1995forEuroScope.h"
#include "RadarScreen.h"
#include "Version.h"

namespace PLM1995forEuroScopeNS
{
    PLM1995forEuroScope::PLM1995forEuroScope() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE), radarScreen(nullptr)
    {
        // Create and register
        radarScreen = new RadarScreenNS::RadarScreen(this);

        // Report Initialised
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    }
    PLM1995forEuroScope::~PLM1995forEuroScope()
    {
        // Tidy up
        if (radarScreen) {
            delete radarScreen;
            radarScreen = nullptr;
        }
    }

    void PLM1995forEuroScope::DisplayMessage(const std::string &message, const std::string &sender)
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

    void PLM1995forEuroScope::OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) {
        if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_GROUND_STATE){
            std::string Callsign = FlightPlan.GetCallsign();
            std::string GroundState = FlightPlan.GetGroundState();
            DisplayMessage(GroundState, Callsign); // Debugging printout
            if (GroundState == "PUSH") {
                EuroScopePlugIn::CRadarTarget RadarTarget = PLM1995forEuroScope::RadarTargetSelect(FlightPlan.GetCallsign());

                // Add aircraft to list of pushers
                PLM1995forEuroScope::pushingBackAircraft[Callsign].position = RadarTarget.GetPosition().GetPosition();
                PLM1995forEuroScope::pushingBackAircraft[Callsign].heading = (double)RadarTarget.GetPosition().GetReportedHeadingTrueNorth();

                DisplayMessage("Heading = " + std::to_string(pushingBackAircraft[Callsign].heading), Callsign); // Debugging printout
            }

            //TODO: Remove from list when changed from pushback
        }
    }

    const std::map<std::string, PLM1995forEuroScope::PushBackData>& PLM1995forEuroScope::GetPushingBackAircraft() const {
        return pushingBackAircraft;
    }

    EuroScopePlugIn::CRadarScreen * PLM1995forEuroScope::OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) {
        DisplayMessage((std::string("On ") + sDisplayName).c_str(), "Activation");
        
        return new RadarScreenNS::RadarScreen(this);
    }

    //TODO: Check for disconnecting aircraft every second
}