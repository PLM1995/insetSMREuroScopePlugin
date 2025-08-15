#pragma once
#include "RadarScreen.h"

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>
#include <map>

namespace PLM1995forEuroScopeNS
{
    class PLM1995forEuroScope : public EuroScopePlugIn::CPlugIn
    {
    public:
        PLM1995forEuroScope();
        ~PLM1995forEuroScope();

        void DisplayMessage(const std::string &message,
                            const std::string &sender = "PLM1995");

        virtual void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType);

        const std::map<std::string, EuroScopePlugIn::CPosition>& GetPushingBackAircraft() const;

        virtual EuroScopePlugIn::CRadarScreen * OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

    private:
        RadarScreenNS::RadarScreen* radarScreen = nullptr;
        std::map<std::string, EuroScopePlugIn::CPosition> pushingBackAircraft;
    };
}
