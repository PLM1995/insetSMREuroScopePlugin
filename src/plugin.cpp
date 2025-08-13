#include "plugin.h"
#include "Version.h"

namespace myPlugIn
{
    myPlugIn::myPlugIn() : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE, PLUGIN_NAME, PLUGIN_VERSION, PLUGIN_AUTHOR, PLUGIN_LICENSE)
    {
        DisplayMessage("Version " + std::string(PLUGIN_VERSION) + " loaded", "Initialisation");
    }
    myPlugIn::~myPlugIn()
    {
    }

    void myPlugIn::DisplayMessage(const std::string &message, const std::string &sender)
    {
        DisplayUserMessage(PLUGIN_NAME, sender.c_str(), message.c_str(), true, false, false, false, false);
    }
}