#pragma once
#include "RadarScreen.h"

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>
#include <map>

enum class PushbackDirection {
    Null = 0,
    Straight = 1,
    Left = 2,
    Right = 3
};

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

        virtual void OnTimer (int Counter);

        struct PushBackData {
            EuroScopePlugIn::CPosition position;
            double heading;
            PushbackDirection direction = PushbackDirection::Null;
        };
        const std::map<const char *, PushBackData>& GetPushingBackAircraft() const;
        const void SetPushingBackDirection(const char *Callsign, PushbackDirection Direction);

        virtual EuroScopePlugIn::CRadarScreen * OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

    private:
        RadarScreenNS::RadarScreen* radarScreen = nullptr;

        std::map<const char *, PushBackData> pushingBackAircraft;
    };
}
