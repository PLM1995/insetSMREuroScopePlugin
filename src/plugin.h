#pragma once

#pragma warning(push, 0)
#include "EuroScopePlugIn.h"
#pragma warning(pop)

#include <string>

namespace myPlugIn
{
    class myPlugIn : public EuroScopePlugIn::CPlugIn
    {
    public:
        myPlugIn();
        ~myPlugIn();

        void DisplayMessage(const std::string &message,
                            const std::string &sender = "myPlugIn");
    };
}
