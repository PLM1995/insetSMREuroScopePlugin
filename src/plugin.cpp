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
        DisplayUserMessage(
            PLUGIN_NAME,
            sender.c_str(),
            message.c_str(),
            true,   // blinking
            false,  // not a warning
            false,  // not high priority
            true,   // persistent
            true    // must be acknowledged
        );
    }

    void myPlugIn::OnTimer(int id) {
        if(id % 2 == 0) {
            DisplayMessage("Meow", "Cat");
        }
        else {
            DisplayMessage("Woof", "Dog");
        }
    }
}