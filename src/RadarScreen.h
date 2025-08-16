#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

//Forward Declaration
namespace PLM1995forEuroScopeNS {
    class PLM1995forEuroScope;
}

namespace RadarScreenNS 
{
    class RadarScreen : public EuroScopePlugIn::CRadarScreen
    {
    public:
        RadarScreen(PLM1995forEuroScopeNS::PLM1995forEuroScope* pluginInstance);

        virtual void OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button);
        virtual void OnRefresh(HDC hDC, int phase);
        virtual void OnAsrContentToBeClosed() override { /* no-op */ }

    private:
        PLM1995forEuroScopeNS::PLM1995forEuroScope* plugin;

        void CirclePoint(HDC hDC, POINT point, int radius);

        EuroScopePlugIn::CPosition PositionFromPosHdgDist(EuroScopePlugIn::CPosition startPosition, double heading, double distance);
    };
}
