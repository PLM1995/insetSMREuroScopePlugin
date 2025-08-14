#include "PLM1995forEuroScope.h"
#include "Version.h"

namespace PLM1995forEuroScope
{
    PLM1995forEuroScope::PLM1995forEuroScope() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
    {
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    }
    PLM1995forEuroScope::~PLM1995forEuroScope()
    {
    }

    void PLM1995forEuroScope::DisplayMessage(const std::string &message, const std::string &sender)
    {
        DisplayUserMessage(
            PLUGIN_NAME,
            sender.c_str(),
            message.c_str(),
            false,  // blinking
            false,  // warning
            false,  // high priority
            false,  // persistent
            false   // requires acknowledgement
        );
    }
/*
    void PLM1995forEuroScope::OnTimer(int secs) {
        if(secs % 10 == 0) {
            DisplayMessage("Woof", "Dog");
        }
        else if(secs % 5 == 0){
            DisplayMessage("Meow", "Cat");
        }
    }
*/
    void PLM1995forEuroScope::OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType) {
        if (DataType == EuroScopePlugIn::CTR_DATA_TYPE_GROUND_STATE){
            std::string Callsign = FlightPlan.GetCallsign();
            std::string GroundState = FlightPlan.GetGroundState();
            DisplayMessage(GroundState, Callsign); // Debugging printout
            if (GroundState == "PUSH") {
                // TODO: Draw warning arc behind aircraft
            }
        }
    }
}