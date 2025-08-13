#include "SectorFileLoad.h"
#include "InsetSMR.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>

namespace SectorFileLoadNS {
    SectorFileLoad::SectorFileLoad(InsetSMRNS::InsetSMR* pluginInstance)
        : plugin(pluginInstance) {}

    std::vector<std::string> SectorFileLoad::splitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(token);
        }
        return tokens;
    }

    int SectorFileLoad::getColourCodeFromName(const std::string& colourName) {
        for (SectorFileLoadNS::SectorFileLoad::ColourDefinition colourDef : SectorFileLoad::colourCodes) {
            if (colourDef.name == colourName) {
                return colourDef.code;
            }
        }
        return 0; // Default colour code if not found
    }

    std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine>* SectorFileLoad::getGeoLines() {
        return &geoLines;
    }

    double SectorFileLoad::dms_to_decimal(std::string coord_str) {
        std::string direction = coord_str.substr(0, 1);
        double degrees = std::stod(coord_str.substr(1, 3));
        double minutes = std::stod(coord_str.substr(5, 2));
        double seconds = std::stod(coord_str.substr(8, 6));
        double decimal = degrees + (minutes / 60.0) + (seconds / 3600.0);
        if (direction == "S" || direction == "W") {
            decimal = -decimal;
        }
        return decimal;
    }

    void SectorFileLoad::updateGeoLineFromStrings(GeoLine& geoLine) {
        geoLine.startLat = dms_to_decimal(geoLine.startLatString);
        geoLine.startLon = dms_to_decimal(geoLine.startLonString);
        geoLine.endLat = dms_to_decimal(geoLine.endLatString);
        geoLine.endLon = dms_to_decimal(geoLine.endLonString);
        geoLine.colourCode = getColourCodeFromName(geoLine.colourName);
        geoLine.colourRed = geoLine.colourCode % 256;
        geoLine.colourGreen = ((geoLine.colourCode - geoLine.colourRed) / 256) % 256;
        geoLine.colourBlue = ((((geoLine.colourCode - geoLine.colourRed) / 256) - geoLine.colourGreen) / 256) % 256;
        return;
    }

    void SectorFileLoad::LoadSectorFile() {
        if (!plugin) return;

        // TODO: Make sector file path configurable
        std::filesystem::path SectorFilePath = "UK/Data/Sector/UK_2026_01.sct";

        std::ifstream sectorFileStream(SectorFilePath);
        if (std::filesystem::exists(SectorFilePath) == false) {
            // File not found
            plugin->DisplayMessage(SectorFilePath.string().c_str(), "Sector file not found at");
            return;
        }
        else {
            plugin->DisplayMessage(SectorFilePath.string().c_str(), "Loading Sector file from");
        }
        if (!sectorFileStream.is_open()) {
            plugin->DisplayMessage("Failed to open sector file.", "Error");
            return;
        }

        // Sector file loading logic
        std::string line = "";
        std::string currentSection =  "";
        std::string activeLoadingAirport = "";
        std::string endOfGeoFirstLine = "S999.00.00.000 E999.00.00.000 S999.00.00.000 E999.00.00.000";
        bool GlasgowGeoLoaded = false;

        // Parse sector file lines
        while (std::getline(sectorFileStream, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == ';') {
                continue;
            }

            // Parse colour definition line
            if (line.find("#define ") != std::string::npos) {
                std::vector<std::string> splitLine = splitString(line, ' ');
                if (splitLine.size() == 3) {
                    SectorFileLoad::ColourDefinition colourDef;
                    colourDef.name = splitLine[1];
                    colourDef.code = std::stoi(splitLine[2]);
                    colourCodes.push_back(colourDef);
                }
                continue;
            }

            // Check for section headers
            if (line.find("[GEO]") != std::string::npos) {
                currentSection = "GEO";
                plugin->DisplayMessage(currentSection, "In Section");
                continue;
            } else if (line.find("[REGIONS]") != std::string::npos) {
                currentSection = "REGIONS";
                plugin->DisplayMessage(currentSection, "In Section");
                continue;
            }

            // Parse relevant section content
            if (currentSection == "GEO") {
                // Parse GEO section
                if (line.find(endOfGeoFirstLine) != std::string::npos) {
//                    plugin->DisplayMessage(line, "Loading GEO Data for");
                    activeLoadingAirport = line.substr(0, 4);
                    continue;
                }
                // TODO: Make airport in use selectable
                if (activeLoadingAirport == "EGPF" && !GlasgowGeoLoaded) {
//                    plugin->DisplayMessage(line, "Loading GEO Data for");
                    std::vector<std::string> splitLine = splitString(line, ' ');
//                    plugin->DisplayMessage(std::to_string(splitLine.size()), "Split line size");
                    if (splitLine.size() == 5) {
                        SectorFileLoad::GeoLine geoLine;
                        geoLine.startLatString = splitLine[0];
                        geoLine.startLonString = splitLine[1];
                        geoLine.endLatString = splitLine[2];
                        geoLine.endLonString = splitLine[3];
                        geoLine.colourName = splitLine[4];
//                        plugin->DisplayMessage(line, "GEO Line strings parsed");
                        updateGeoLineFromStrings(geoLine);
//                        plugin->DisplayMessage(line, "Parsed GEO Line");
                        
                        // Store this GEO line in the vector
                        geoLines.push_back(geoLine);
                        continue;
                    }
                    else {
                        plugin->DisplayMessage(line, "Invalid GEO line format");
                        continue;
                    }
                }
                // FIXME: This is a temporary measure to only load Glasgow SMR GEO data
                else if (geoLines.size() > 0) {
                    GlasgowGeoLoaded = true;
                }
            }
            else if (currentSection == "REGIONS") {
                // Parse REGIONS section
            }
        }

        sectorFileStream.close();

        plugin->DisplayMessage("Sector file loaded successfully.", "Success");
    }
}