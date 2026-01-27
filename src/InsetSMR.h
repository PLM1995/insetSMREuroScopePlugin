#pragma once
#include "RadarScreen.h"
#include "SectorFileLoad.h"

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

        virtual void OnTimer (int Counter);

        virtual EuroScopePlugIn::CRadarScreen * OnRadarScreenCreated(const char * sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

    private:
        RadarScreenNS::RadarScreen* radarScreen = nullptr;
        SectorFileLoadNS::SectorFileLoad* sectorFileLoader = nullptr;
    };
}
