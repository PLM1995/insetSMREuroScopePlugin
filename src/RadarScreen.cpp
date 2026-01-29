/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#include "constants.h"
#include "RadarScreen.h"
#include "InsetSMR.h"
#include "SectorFileLoad.h"

namespace RadarScreenNS{
    RadarScreen::RadarScreen(InsetSMRNS::InsetSMR* pluginInstance) : plugin(pluginInstance) {}

    void RadarScreen::OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button)
    {
        // Clicks to select aircraft in inset
        if (Button == EuroScopePlugIn::BUTTON_LEFT && ObjectType == SELECTINSETSMRAIRCRAFT) {
            std::vector<InsetSMRNS::InsetSMR::RadarTargetSnapshot> targets;
            if (plugin) targets = plugin->getActiveRadarTargetSnapshots();
            for (const auto &rt : targets) {
                if (!rt.valid) {
                    continue;
                }
                if (rt.callsign == sObjectId) {
                    if (plugin) {
                        auto esFlightPlan = plugin->FlightPlanSelect(rt.callsign.c_str());

//                        if (plugin) plugin->LogEvent("Selecting aircraft from inset: " + rt.callsign);

                        if (esFlightPlan.IsValid()) {
                            plugin->SelectAircraftFromFlightPlan(esFlightPlan);
                        }

                        // FIXME: Shows "Unidentified" even when valid?
                    }
                    break;
                }
            }
        }
    }
    
    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
//            if (plugin) plugin->LogEvent("OnRefresh start");
            // Top left inset rectangle
            int insetLeftPosition = 10;
            int insetTopPosition = 50;
            int insetWidth = 576;
            int insetHeight = 324;
            // TODO: Make moveable
            RECT insetRect = { insetLeftPosition, insetTopPosition, insetLeftPosition + insetWidth, insetTopPosition + insetHeight };

            // Draw border
            Rectangle(hDC, insetRect.left, insetRect.top, insetRect.right, insetRect.bottom);

            // Set clipping region to inset area
            HRGN clipRgn = CreateRectRgnIndirect(&insetRect);

            // Save old clip region
            int savedDC = SaveDC(hDC);
            SelectClipRgn(hDC, clipRgn);

            // FIXME: Change from hard coded values to configurable / based on airport
            double minLon = -4.453889;
            double maxLon = -4.416389;
            double minLat = 55.861389;
            double maxLat = 55.880833;
        
            double lonRange = maxLon - minLon;
            double latRange = maxLat - minLat;
            // Avoid divide‑by‑zero
            if (lonRange <= 0.0 || latRange <= 0.0){
                plugin->DisplayMessage("Invalid lat/lon range for inset drawing", "RadarScreen");
                return;
            }

            // Draw loaded GEO lines and REGIONS
            if (plugin) {
                auto sectorFileLoader = plugin->GetSectorFileLoader();
                if (sectorFileLoader) {
                    // Draw each REGION
                    const auto regions = sectorFileLoader->getRegions();
                    for (const auto &region : *regions) {
                        // Need at least 3 points to draw a region
                        if (region.boundaryCoords.size() < 3) {
                            continue; 
                        }

                        // Create and select a brush
                        HBRUSH hBrush = CreateSolidBrush(RGB(region.colourRed, region.colourGreen, region.colourBlue));
                        HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, hBrush);

                        // Create/select a pen
                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(region.colourRed, region.colourGreen, region.colourBlue));
                        HPEN oldPen = (HPEN)SelectObject(hDC, hPen);

                        // Load region points
                        std::vector<POINT> pts;
                        pts.reserve(region.boundaryCoords.size() + 1);

                        // Save first point
                        double firstXNorm = (region.boundaryCoords[0].lon - minLon) / lonRange;
                        double firstYNorm = (region.boundaryCoords[0].lat - minLat) / latRange;
                        POINT firstPt = {
                            insetLeftPosition + static_cast<LONG>(firstXNorm * insetWidth),
                            insetTopPosition  + insetHeight - static_cast<LONG>(firstYNorm * insetHeight)
                        };
                        pts.push_back(firstPt);

                        // Save subsequent points
                        for (size_t i = 1; i < region.boundaryCoords.size(); ++i) {
                            double xNorm = (region.boundaryCoords[i].lon - minLon) / lonRange;
                            double yNorm = (region.boundaryCoords[i].lat - minLat) / latRange;
                            POINT pt = {
                                insetLeftPosition + static_cast<LONG>(xNorm * insetWidth),
                                insetTopPosition  + insetHeight - static_cast<LONG>(yNorm * insetHeight)
                            };
                            pts.push_back(pt);
                        }

                        // Close the region by adding the first point at the end
                        pts.push_back(firstPt);

                        // Draw the polygon
                        Polygon(hDC, pts.data(), static_cast<int>(pts.size()));

                        // Cleanup
                        SelectObject(hDC, oldBrush);
                        SelectObject(hDC, oldPen);
                        DeleteObject(hBrush);
                        DeleteObject(hPen);
                    }

                    // Draw each GEO line
                    auto geoLines = sectorFileLoader->getGeoLines();
                    for (const auto &geoLine : *geoLines) {
                        double startXNorm = (geoLine.startLon - minLon) / lonRange;
                        double startYNorm = (geoLine.startLat - minLat) / latRange;
                        double endXNorm   = (geoLine.endLon   - minLon) / lonRange;
                        double endYNorm   = (geoLine.endLat   - minLat) / latRange;

                        POINT startPt = {
                            insetLeftPosition + static_cast<LONG>(startXNorm * insetWidth),
                            insetTopPosition  + insetHeight - static_cast<LONG>(startYNorm * insetHeight)
                        };

                        POINT endPt = {
                            insetLeftPosition + static_cast<LONG>(endXNorm * insetWidth),
                            insetTopPosition  + insetHeight - static_cast<LONG>(endYNorm * insetHeight)
                        };

                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(geoLine.colourRed, geoLine.colourGreen, geoLine.colourBlue));
                        HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

                        MoveToEx(hDC, startPt.x, startPt.y, NULL);
                        LineTo(hDC, endPt.x, endPt.y);

                        SelectObject(hDC, hOldPen);
                        DeleteObject(hPen);
                    }
                }
            }

            // Draw aircraft symbols
            std::vector<InsetSMRNS::InsetSMR::RadarTargetSnapshot> targets;
            if (plugin) targets = plugin->getActiveRadarTargetSnapshots();
//            if (plugin) plugin->LogEvent(std::string("OnRefresh: target snapshot size=") + std::to_string(targets.size()));
            for (const auto &rt : targets) {
                if (!rt.valid) {
                    continue;
                }

                double acLon = rt.lon;
                double acLat = rt.lat;
                std::string acCallsign = rt.callsign;

                // Check if within inset bounds
                if (acLon < minLon || acLon > maxLon || acLat < minLat || acLat > maxLat) {
                    continue;
                }

                double acXNorm = (acLon - minLon) / lonRange;
                double acYNorm = (acLat - minLat) / latRange;

                POINT acPt = {
                    insetLeftPosition + static_cast<LONG>(acXNorm * insetWidth),
                    insetTopPosition  + insetHeight - static_cast<LONG>(acYNorm * insetHeight)
                };

                // Draw aircraft symbol
                // Create and select a brush
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0)); // Yellowish color
                HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, hBrush);

                // Create/select a pen
                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 0)); // Yellowish color
                HPEN oldPen = (HPEN)SelectObject(hDC, hPen);

                // Draw as a filled polygon
                std::vector<POINT> pts = {};
                pts.push_back({ acPt.x - 2, acPt.y });
                pts.push_back({ acPt.x,     acPt.y + 2 });
                pts.push_back({ acPt.x + 2, acPt.y });
                pts.push_back({ acPt.x,     acPt.y - 2 });
                pts.push_back({ acPt.x - 2, acPt.y });
                Polygon(hDC, pts.data(), static_cast<int>(pts.size()));

                // Register the aircraft as clickable
                RECT ClickableArea = RECT({ acPt.x - 4, acPt.y - 4, acPt.x + 4, acPt.y + 4 });
                AddScreenObject(SELECTINSETSMRAIRCRAFT, acCallsign.c_str(), ClickableArea, false, "");

                // Draw callsign
                HFONT hFont = CreateFontA(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                          ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                          DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                          "Courier New");
                HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
                SetTextColor(hDC, RGB(255, 255, 0)); // Yellowish color
                SetBkMode(hDC, TRANSPARENT); // Transparent background
                TextOutA(hDC, acPt.x + 5, acPt.y - 8, acCallsign.c_str(), static_cast<int>(acCallsign.length()));

                // Cleanup
                SelectObject(hDC, oldBrush);
                SelectObject(hDC, oldPen);
                SelectObject(hDC, hOldFont);
                DeleteObject(hFont);
                DeleteObject(hBrush);
                DeleteObject(hPen);
            }

//            if (plugin) plugin->LogEvent("OnRefresh end");
            // Clear clip region
            RestoreDC(hDC, savedDC);
            DeleteObject(clipRgn);
        }
    }
}