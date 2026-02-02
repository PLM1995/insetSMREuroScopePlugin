/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#pragma once
#include <string>
#include <vector>

namespace InsetSMRNS { class InsetSMR; }

namespace SectorFileLoadNS {
    class SectorFileLoad {
    public:
        SectorFileLoad(InsetSMRNS::InsetSMR* pluginInstance);
        virtual void LoadSectorFile();

        struct Colour {
            std::string name;
            int code;
            int Red;
            int Green;
            int Blue;
        };

        struct GeoLine {
            std::string startLatString;
            std::string startLonString;
            std::string endLatString;
            std::string endLonString;
            double startLat;
            double startLon;
            double endLat;
            double endLon;
            Colour colour;
        };

        struct coordinate {
            double lat;
            double lon;
        };

        struct Region {
            std::string name;
            std::vector<coordinate> boundaryCoords;
            Colour colour;
        };

        struct Label {
            std::string label;
            std::string category;
            coordinate position;
            Colour colour;
        };

        std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine>* getGeoLines();

        std::vector<SectorFileLoadNS::SectorFileLoad::Region>* getRegions();

        std::vector<SectorFileLoadNS::SectorFileLoad::Label>* getLabels();

    private:
        InsetSMRNS::InsetSMR* plugin = nullptr;
        
        std::vector<std::string> splitString(const std::string& str, char delimiter);
        
        std::vector<SectorFileLoadNS::SectorFileLoad::Colour> colours;
        
        void updateColour(Colour& colour);

        Colour getColourFromName(const std::string& colourName);
        
        bool updateGeoLineFromStrings(GeoLine& geoLine);
        
        void updateRegionFromStrings(Region& region);

        void updateLabelFromStrings(Label& label);

        bool try_dms_to_decimal(const std::string& coord_str, double &out);

        std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine> geoLines;

        std::vector<Region> regions;

        std::vector<Label> labels;
        
        double dms_to_decimal(std::string coord_str);
        
        bool GlasgowGeoLoaded = false;
    };
}