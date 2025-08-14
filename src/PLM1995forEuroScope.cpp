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
            true,   // blinking
            false,  // warning
            false,  // high priority
            true,   // persistent
            true    // requires acknowledgement
        );
    }

    void PLM1995forEuroScope::OnTimer(int secs) {
        if(secs % 2 == 0) {
            DisplayMessage("Meow", "Cat");
        }
        else {
            DisplayMessage("Woof", "Dog");
        }
    }
}