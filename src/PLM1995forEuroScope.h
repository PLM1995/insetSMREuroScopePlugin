#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>

namespace PLM1995forEuroScope
{
    class PLM1995forEuroScope : public EuroScopePlugIn::CPlugIn
    {
    public:
        PLM1995forEuroScope();
        ~PLM1995forEuroScope();

        void DisplayMessage(const std::string &message,
                            const std::string &sender = "PLM1995");

//        virtual void OnTimer(int secs);
        virtual void OnFlightPlanControllerAssignedDataUpdate(EuroScopePlugIn::CFlightPlan FlightPlan, int DataType);
    };
}
