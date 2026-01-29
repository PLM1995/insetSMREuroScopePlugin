/* Simple POD for inset view coordinates */
#pragma once

namespace InsetSMRNS {
    struct ViewCoordinates {
        double minViewLon;
        double minViewLat;
        double maxViewLon;
        double maxViewLat;
    };
}
