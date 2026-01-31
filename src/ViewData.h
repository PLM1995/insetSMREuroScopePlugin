/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

/* Simple POD for inset view coordinates and modes */
#pragma once

namespace InsetSMRNS {
    struct ViewCoordinates {
        double minViewLon;
        double minViewLat;
        double maxViewLon;
        double maxViewLat;
    };

    enum VIEWMODE {
        AIRPORT,
        RUNWAY,
        HOLDINGAREA
    };
}
