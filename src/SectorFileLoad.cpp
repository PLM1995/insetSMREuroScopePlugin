/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#include "SectorFileLoad.h"
#include "InsetSMR.h"
#include <fstream>
#include <filesystem>
#include <string>
#include <sstream>
#include <vector>
#include <thread>
#include <chrono>
#include <Windows.h>
#include <cerrno>
#include <cstring>

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

        plugin->DisplayMessage(colourName, "Unknown colour name");
        return 0; // Default colour code if not found (black)
    }

    std::vector<SectorFileLoadNS::SectorFileLoad::GeoLine>* SectorFileLoad::getGeoLines() {
        return &geoLines;
    }

    std::vector<SectorFileLoadNS::SectorFileLoad::Region>* SectorFileLoad::getRegions() {
        return &regions;
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

    bool SectorFileLoad::try_dms_to_decimal(const std::string& coord_str, double &out) {
        try {
            if (coord_str.empty()) {
                if (plugin) plugin->LogEvent(std::string("try_dms_to_decimal: empty coord string"));
                return false;
            }
            // Minimal expected length: 1 (dir) + 3 (deg) + 2 (min) + 6 (sec) = 12, plus separators/format may vary
            if (coord_str.size() < 12) {
                if (plugin) plugin->LogEvent(std::string("try_dms_to_decimal: coord too short: '") + coord_str + "'");
                return false;
            }
            out = dms_to_decimal(coord_str);
            return true;
        } catch (const std::exception &ex) {
            if (plugin) plugin->LogEvent(std::string("try_dms_to_decimal: failed for '") + coord_str + "' with: " + ex.what());
            return false;
        } catch (...) {
            if (plugin) plugin->LogEvent(std::string("try_dms_to_decimal: unknown error for '") + coord_str + "'" );
            return false;
        }
    }

    bool SectorFileLoad::updateGeoLineFromStrings(GeoLine& geoLine) {
        double v1, v2, v3, v4;
        if (!try_dms_to_decimal(geoLine.startLatString, v1)) return false;
        if (!try_dms_to_decimal(geoLine.startLonString, v2)) return false;
        if (!try_dms_to_decimal(geoLine.endLatString, v3)) return false;
        if (!try_dms_to_decimal(geoLine.endLonString, v4)) return false;

        geoLine.startLat = v1;
        geoLine.startLon = v2;
        geoLine.endLat = v3;
        geoLine.endLon = v4;
        geoLine.colourCode = getColourCodeFromName(geoLine.colourName);
        geoLine.colourRed = geoLine.colourCode % 256;
        geoLine.colourGreen = ((geoLine.colourCode - geoLine.colourRed) / 256) % 256;
        geoLine.colourBlue = ((((geoLine.colourCode - geoLine.colourRed) / 256) - geoLine.colourGreen) / 256) % 256;
        return true;
    }

    void SectorFileLoad::updateRegionFromStrings(Region& region) {
//        plugin->DisplayMessage(region.name, "Updating Region From String, Region name");
        region.colourCode = getColourCodeFromName(region.colourName);
        region.colourRed = region.colourCode % 256;
        region.colourGreen = ((region.colourCode - region.colourRed) / 256) % 256;
        region.colourBlue = ((((region.colourCode - region.colourRed) / 256) - region.colourGreen) / 256) % 256;
        return;
    }

    void SectorFileLoad::LoadSectorFile() {
        try {
            if (plugin) plugin->LogEvent("LoadSectorFile: entry");
            InsetSMRNS::InsetSMR::Airport activeAirport;

            if (plugin) {
                try {
                    activeAirport = plugin->getActiveAirport();
                    plugin->LogEvent(std::string("LoadSectorFile: got activeAirport=") + activeAirport.ICAO);
                } catch (const std::exception &ex) {
                    plugin->LogEvent(std::string("LoadSectorFile: getActiveAirport threw: ") + ex.what());
                    return;
                }
            } else {
                return;
            }

        // Clear any previously loaded data to ensure consistent state
        geoLines.clear();
        regions.clear();
        colourCodes.clear();

        if (plugin) {
            plugin->LogEvent(std::string("LoadSectorFile: activeAirport=") + activeAirport.ICAO);
            plugin->LogEvent(std::string("LoadSectorFile: RelevantGeoNames size=") + std::to_string(activeAirport.RelevantGeoNames.size()));
            // plugin->LogEvent("LoadSectorFile: RelevantGeoNames are:");
            // for (std::string relevantGeo : activeAirport.RelevantGeoNames) {
            //     plugin->LogEvent(relevantGeo);
            // }
            plugin->LogEvent(std::string("LoadSectorFile: RelevantRegionNames size=") + std::to_string(activeAirport.RelevantRegionNames.size()));
        }

        // Find the path to the UK sector file
        std::string sectorFileDirPath = "UK/Data/Sector";
        std::filesystem::path SectorFilePath;
        for (const auto & entry : std::filesystem::directory_iterator(sectorFileDirPath)) {
            std::string fileName = entry.path().string().substr(entry.path().string().find_last_of("/\\") + 1);
            std::string fileExtention = fileName.substr(fileName.find_last_of("."));
            if(fileExtention == ".sct" && fileName.find("UK") != std::string::npos) {
                SectorFilePath = entry.path().string();
                // NOTE: Don't continue as we want to use the last (most recent) relevant sector file in the folder
            }
        }

        // Retry loop to handle transient locks (antivirus, OneDrive sync, other process)
        const int maxAttempts = 6;
        const auto delay = std::chrono::milliseconds(250);
        std::ifstream sectorFileStream;
        bool opened = false;
        for (int attempt = 1; attempt <= maxAttempts; ++attempt) {
            if (plugin) plugin->LogEvent(std::string("LoadSectorFile: open attempt ") + std::to_string(attempt));
            try {
                if (!std::filesystem::exists(SectorFilePath)) {
                    if (plugin) plugin->DisplayMessage(SectorFilePath.string().c_str(), "Sector file not found at");
                    return;
                }
                if (plugin) plugin->LogEvent(std::string("LoadSectorFile: exists returned true for ") + SectorFilePath.string());
            } catch (const std::filesystem::filesystem_error &fex) {
                if (plugin) plugin->LogEvent(std::string("Attempt ") + std::to_string(attempt) + ": filesystem::exists threw: " + fex.what());
                if (attempt < maxAttempts) std::this_thread::sleep_for(delay);
                continue;
            }

            // Try to open with ifstream
            sectorFileStream.open(SectorFilePath, std::ios::in);
            if (sectorFileStream.is_open()) {
                opened = true;
                break;
            }
            else {
                int err = errno;
                if (plugin) plugin->LogEvent(std::string("Attempt ") + std::to_string(attempt) + ": ifstream open failed, errno=" + std::to_string(err) + ", strerror=" + std::string(std::strerror(err)));
            }

            // As a fallback, try CreateFile with sharing to probe file availability
            HANDLE h = CreateFileA(SectorFilePath.string().c_str(), GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                    NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                // Try opening stream again
                sectorFileStream.open(SectorFilePath, std::ios::in);
                if (sectorFileStream.is_open()) { opened = true; break; }
            } else {
                DWORD err = GetLastError();
                if (plugin) plugin->LogEvent(std::string("CreateFile attempt ") + std::to_string(attempt) + " failed with GetLastError=" + std::to_string(err));
            }

            if (attempt < maxAttempts) std::this_thread::sleep_for(delay);
        }

        if (!opened) {
            if (plugin) plugin->DisplayMessage("Failed to open sector file after retries.", "Error");
            if (plugin) plugin->LogEvent(std::string("Failed to open sector file: ") + SectorFilePath.string());
            return;
        }
        if (plugin) plugin->DisplayMessage(SectorFilePath.string().c_str(), "Loading Sector file from");

        // Sector file loading logic
        std::string line = "";
        std::string currentSection =  "";
        std::string currentGeoName = "";
        std::string startOfNewGeoSectionTest = "S999.00.00.000 E999.00.00.000 S999.00.00.000 E999.00.00.000";
        bool loadThisGeo = false;
        bool relevantGeoLoaded = false;
        std::string currentRegionName = "";
        SectorFileLoad::Region loadingRegion;
        bool loadThisRegion = false;

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
//                plugin->DisplayMessage(currentSection, "In Section");
                continue;
            } else if (line.find("[REGIONS]") != std::string::npos) {
                currentSection = "REGIONS";
//                plugin->DisplayMessage(currentSection, "In Section");
                continue;
            }

            // Parse relevant section content
            // Parse GEO section
            if (currentSection == "GEO") {
                // New GEO definition
                if (line.find(startOfNewGeoSectionTest) != std::string::npos) {
                    loadThisGeo = false; // Assume not going to read it ntil proven otherwise
//                    if (plugin) plugin->LogEvent("Looking at " + line);
                    // Decide whether to load this geo based on activeAirport.RelevantGeoNames
                    for (const std::string &relevantGeoName : activeAirport.RelevantGeoNames) {
//                        if (plugin) plugin->LogEvent("Checking for " + relevantGeoName);
                        // Logic santises to check, e.g. "EGPF Glasgow FAVA" ignored  if"EGPF Glasgow" searched
                        if (line.find(relevantGeoName) != std::string::npos) {
                            loadThisGeo = true;
                            std::string sanitisedLine = line;
                            sanitisedLine = sanitisedLine.erase(line.find(startOfNewGeoSectionTest), startOfNewGeoSectionTest.size());
                            sanitisedLine = sanitisedLine.erase(line.find(relevantGeoName), relevantGeoName.size());
                            std::vector<std::string> splitSanitisedLine = splitString(sanitisedLine, ' ');
                            for (const std::string& nonWhitespace : splitSanitisedLine) {
                                if (nonWhitespace.size() > 0) {
                                    loadThisGeo = false;
//                                    if (plugin) plugin->LogEvent("Ignoring: " + line + " GEO as something other than " + relevantGeoName);
                                    for (char nonWhitespaceChar : nonWhitespace) {
//                                        if (plugin) plugin->LogEvent("Extra Character: \"" + std::string(1, nonWhitespaceChar) + "\" found");
                                    }
                                }
                            }
                            if (loadThisGeo){
                                if (plugin) plugin->LogEvent("Planning to capture the Geo beginning: " + line);
                            }
                        }                        
                    }
                    continue; // Read next line
                }

                if (loadThisGeo) {
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
                        if (!updateGeoLineFromStrings(geoLine)) {
                            if (plugin) plugin->LogEvent(std::string("Skipping GEO line due to parse error: ") + line);
                            continue;
                        }
                        // Store this GEO line in the vector
                        geoLines.push_back(geoLine);
                        continue;
                    }
                    else {
                        plugin->DisplayMessage(line, "Invalid GEO line format");
                        continue;
                    }
                }
            }

            // Parse REGIONS section
            else if (currentSection == "REGIONS") {
                std::vector<std::string> splitLine = splitString(line, ' ');

                // New REGION definition (REGIONNAME <name>)
                if (!splitLine.empty() && splitLine[0].find("REGIONNAME") != std::string::npos) {
                    // Save the previous region before starting a new one
                    if (loadingRegion.name != "") {
//                        if (plugin) plugin->LogEvent(std::string("Pushing region ") + loadingRegion.name + ", points=" + std::to_string(loadingRegion.boundaryCoords.size()));
                        regions.push_back(loadingRegion);
                        loadingRegion = SectorFileLoad::Region(); // Reset loadingRegion
                    }
                    currentRegionName = (splitLine.size() > 1) ? splitLine[1] : std::string("");

                    // Decide whether to load this region based on activeAirport.RelevantRegionNames
                    loadThisRegion = false;
                    for (const std::string &relevantRegionName : activeAirport.RelevantRegionNames) {
                        if (relevantRegionName == currentRegionName) {
                            loadThisRegion = true;
                            break;
                        }
                    }
                    continue; // Move to read next line
                }

                // Only process region coordinate/colour lines if this region is relevant
                if (!loadThisRegion) {
                    continue;
                }

                // Handle lines that may include a leading blank token due to spacing
                std::string firstToken = (splitLine.size() > 0) ? splitLine[0] : std::string("");
                std::string colourToken;
                size_t latIdx = 0, lonIdx = 1;

                if (firstToken == "") {
                    // Leading space produced empty first token: expect tokens ["", lat, lon, ...]
                    if (splitLine.size() > 2) {
                        colourToken = std::string("");
                        latIdx = 1;
                        lonIdx = 2;
                    } else {
                        if (plugin) plugin->LogEvent(std::string("Skipping invalid REGION line (too few tokens): ") + line);
                        continue;
                    }
                } else {
                    if (splitLine.size() > 2) {
                        // Format: <ColourName> <Lat> <Lon>
                        colourToken = splitLine[0];
                        latIdx = 1;
                        lonIdx = 2;
                    } else if (splitLine.size() == 2) {
                        // Format: <Lat> <Lon>
                        colourToken = std::string("");
                        latIdx = 0;
                        lonIdx = 1;
                    } else {
                        if (plugin) plugin->LogEvent(std::string("Skipping invalid REGION line (unexpected format): ") + line);
                        continue;
                    }
                }

                // Initialize region metadata on first coordinate of the region
                if (loadingRegion.name == "") {
                    loadingRegion.name = currentRegionName;
                }
                if (!colourToken.empty() && loadingRegion.colourName.empty()) {
                    loadingRegion.colourName = colourToken;
                    updateRegionFromStrings(loadingRegion);
                }

                double lat, lon;
                if (try_dms_to_decimal(splitLine[latIdx], lat) && try_dms_to_decimal(splitLine[lonIdx], lon)) {
                    loadingRegion.boundaryCoords.push_back({lat, lon});
                } else {
                    if (plugin) plugin->LogEvent(std::string("Skipping REGION coord due to parse error: ") + line);
                }
                continue;
            }
        }

        // If we finished file while building a region, save it now
        if (loadingRegion.name != "") {
//            if (plugin) plugin->LogEvent(std::string("Pushing final region ") + loadingRegion.name + ", points=" + std::to_string(loadingRegion.boundaryCoords.size()));
            regions.push_back(loadingRegion);
            loadingRegion = SectorFileLoad::Region();
        }

        sectorFileStream.close();

        // // Debugging Logs
        // if (plugin) {
        //     plugin->LogEvent(std::string("LoadSectorFile: geoLines=") + std::to_string(geoLines.size()) + ", regions=" + std::to_string(regions.size()) + ", colours=" + std::to_string(colourCodes.size()));
        //     // Log first region names for debugging
        //     for (size_t i = 0; i < regions.size() && i < 5; ++i) {
        //         plugin->LogEvent(std::string("Region[") + std::to_string(i) + "]=" + regions[i].name);
        //     }
        // }

        plugin->DisplayMessage("Sector file loaded", "Success");
        }
        catch (const std::exception &ex) {
            if (plugin) plugin->LogEvent(std::string("LoadSectorFile exception: ") + ex.what());
        }
        catch (...) {
            if (plugin) plugin->LogEvent(std::string("LoadSectorFile unknown exception"));
        }
    }
}