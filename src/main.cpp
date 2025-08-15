#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "PLM1995forEuroScope.h"

std::unique_ptr<EuroScopePlugIn::CPlugIn> Plugin;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance)
{
  Plugin.reset(new PLM1995forEuroScopeNS::PLM1995forEuroScope());
  *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {}
