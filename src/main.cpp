/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

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
