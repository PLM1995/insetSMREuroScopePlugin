/*
 * Copyright (C) 2026 @PLM1995
 * This program is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 */

#include "RadarScreen.h"
#include "InsetSMR.h"
#include "SectorFileLoad.h"

namespace RadarScreenNS{
    RadarScreen::RadarScreen(InsetSMRNS::InsetSMR* pluginInstance) : plugin(pluginInstance) {}

    void RadarScreen::OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button)
    {
        // TODO: Implement clicks to select aircraft in inset
    }
    
    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
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

            // Draw loaded GEO lines
            if (plugin) {
                auto sectorFileLoader = plugin->GetSectorFileLoader();
                if (sectorFileLoader) {
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

            // Clear clip region
            RestoreDC(hDC, savedDC);
            DeleteObject(clipRgn);
        }
    }
}