#pragma once
#include <string>
#include <vector>

namespace InsetSMRNS { class InsetSMR; }

namespace SectorFileLoadNS {
    class SectorFileLoad {
    public:
        SectorFileLoad(InsetSMRNS::InsetSMR* pluginInstance);
        virtual void LoadSectorFile();

        struct GeoLine {
            std::string startLatString;
            std::string startLonString;
            std::string endLatString;
            std::string endLonString;
            double startLat;
            double startLon;
            double endLat;
            double endLon;
            std::string colourName;
            int colourCode;
            int colourRed;
            int colourGreen;
            int colourBlue;
        };

        std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine>* getGeoLines();

    private:
        InsetSMRNS::InsetSMR* plugin = nullptr;
        std::vector<std::string> splitString(const std::string& str, char delimiter);
        struct ColourDefinition {
            std::string name;
            int code;
        };
        std::vector<SectorFileLoadNS::SectorFileLoad::ColourDefinition> colourCodes;
        int getColourCodeFromName(const std::string& colourName);
        void updateGeoLineFromStrings(GeoLine& geoLine);
        std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine> geoLines;
        double dms_to_decimal(std::string coord_str);
        bool GlasgowGeoLoaded = false;
    };
}