#include <memory>
#pragma warning(push, 0)
#include <EuroScopePlugIn.h>
#pragma warning(pop)

#include "InsetSMR.h"

std::unique_ptr<InsetSMRNS::InsetSMR> Plugin;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn **ppPlugInInstance)
{
  Plugin.reset(new InsetSMRNS::InsetSMR());
  *ppPlugInInstance = Plugin.get();
}

void __declspec(dllexport) EuroScopePlugInExit(void) {
  Plugin.reset(nullptr);
}
