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
        // Click to select aircraft in inset
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

        // Click to hide inset
        else if (Button == EuroScopePlugIn::BUTTON_LEFT && ObjectType == HIDEINSETSMRBUTTON) {
            SetShowingInsetSMR(false);
            if (plugin) {
                plugin->DisplayMessage("InsetSMR hidden. To show again, use \".InsetSMR Show\"", "InsetSMR");
            }
        }

        // Click to minimise inset
        else if (Button == EuroScopePlugIn::BUTTON_LEFT && ObjectType == MINIMISEINSETSMRBUTTON) {
            ToggleInsetSMRMinimised();
        }
    }
    
    void RadarScreen::OnMoveScreenObject ( int ObjectType, const char * sObjectId, POINT Pt, RECT Area, bool Released ) {
        if (ObjectType == INSETSMROBJECT && std::strcmp(sObjectId, "InsetSMR") == 0) {
            insetTopLeftPosition.x = Pt.x - (Area.right - Area.left) / 2;
            insetTopLeftPosition.y = Pt.y  - (Area.bottom - Area.top) / 2;

            if (Released) {
                // TODO: persist the final position
            }
        }
    }

    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS && isShowingInsetSMR()) {
//            if (plugin) plugin->LogEvent("OnRefresh start");

            // Top left inset rectangle
            int normalInsetWidth = normalInsetWidth1 * getScaleFactor();
            int normalInsetHeight = normalInsetHeight1 * getScaleFactor();
            int minimisedInsetWidth = 100;
            int minimisedInsetHeight = 30; 
            
            // Position and size inset based on minimisation and drag
            RECT insetRect;
            insetRect.left = insetTopLeftPosition.x;
            insetRect.top =  insetTopLeftPosition.y;
            if (!isInsetSMRMinimised()) {
                insetRect.right  = insetRect.left + normalInsetWidth;
                insetRect.bottom = insetRect.top + normalInsetHeight;
            } else {
                insetRect.right  = insetRect.left + minimisedInsetWidth;
                insetRect.bottom = insetRect.top + minimisedInsetHeight;
            }

            // Allow for moveability
            AddScreenObject(INSETSMROBJECT, "InsetSMR", insetRect, true, "");

            // Set clipping region to inset area
            HRGN clipRgn = CreateRectRgnIndirect(&insetRect);

            // Save old clip region
            int savedDC = SaveDC(hDC);
            SelectClipRgn(hDC, clipRgn);

            // Get the area to view
            InsetSMRNS::ViewCoordinates viewArea = getInsetViewArea();

            // Evaluate view ranges (for scaling)
            double lonRange = viewArea.maxViewLon - viewArea.minViewLon;
            double latRange = viewArea.maxViewLat - viewArea.minViewLat;

            // Avoid divide‑by‑zero
            if (lonRange <= 0.0 || latRange <= 0.0){
                plugin->DisplayMessage("Invalid lat/lon range for inset drawing", "RadarScreen");
                return;
            }

            // Draw loaded SMR background (GEO lines and REGIONS)
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
                        HBRUSH hBrush = CreateSolidBrush(RGB(region.colour.Red, region.colour.Green, region.colour.Blue));
                        HBRUSH oldBrush = (HBRUSH)SelectObject(hDC, hBrush);

                        // Create/select a pen
                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(region.colour.Red, region.colour.Green, region.colour.Blue));
                        HPEN oldPen = (HPEN)SelectObject(hDC, hPen);

                        // Load region points
                        std::vector<POINT> pts;
                        pts.reserve(region.boundaryCoords.size() + 1);

                        // Save first point
                        double firstXNorm = (region.boundaryCoords[0].lon - viewArea.minViewLon) / lonRange;
                        double firstYNorm = (region.boundaryCoords[0].lat - viewArea.minViewLat) / latRange;
                        POINT firstPt = {
                            insetTopLeftPosition.x + static_cast<LONG>(firstXNorm * normalInsetWidth),
                            insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(firstYNorm * normalInsetHeight)
                        };
                        pts.push_back(firstPt);

                        // Save subsequent points
                        for (size_t i = 1; i < region.boundaryCoords.size(); ++i) {
                            double xNorm = (region.boundaryCoords[i].lon - viewArea.minViewLon) / lonRange;
                            double yNorm = (region.boundaryCoords[i].lat - viewArea.minViewLat) / latRange;
                            POINT pt = {
                                insetTopLeftPosition.x + static_cast<LONG>(xNorm * normalInsetWidth),
                                insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(yNorm * normalInsetHeight)
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
                        double startXNorm = (geoLine.startLon - viewArea.minViewLon) / lonRange;
                        double startYNorm = (geoLine.startLat - viewArea.minViewLat) / latRange;
                        double endXNorm   = (geoLine.endLon   - viewArea.minViewLon) / lonRange;
                        double endYNorm   = (geoLine.endLat   - viewArea.minViewLat) / latRange;

                        POINT startPt = {
                            insetTopLeftPosition.x + static_cast<LONG>(startXNorm * normalInsetWidth),
                            insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(startYNorm * normalInsetHeight)
                        };

                        POINT endPt = {
                            insetTopLeftPosition.x + static_cast<LONG>(endXNorm * normalInsetWidth),
                            insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(endYNorm * normalInsetHeight)
                        };

                        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(geoLine.colour.Red, geoLine.colour.Green, geoLine.colour.Blue));
                        HPEN hOldPen = (HPEN)SelectObject(hDC, hPen);

                        MoveToEx(hDC, startPt.x, startPt.y, NULL);
                        LineTo(hDC, endPt.x, endPt.y);

                        // Cleanup
                        SelectObject(hDC, hOldPen);
                        DeleteObject(hPen);
                    }

                    // Draw each LABEL if required
                    if (areLabelsShown()) {
                        auto labels = sectorFileLoader->getLabels();
                        for (const auto &label : *labels) {
                            // Calculate position
                            double labelXNorm = (label.position.lon - viewArea.minViewLon) / lonRange;
                            double labelYNorm = (label.position.lat - viewArea.minViewLat) / latRange;
                            POINT labelPt = {
                                insetTopLeftPosition.x + static_cast<LONG>(labelXNorm * normalInsetWidth),
                                insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(labelYNorm * normalInsetHeight)
                            };

                            // Draw Label
                            HFONT hFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                            ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                            "Arial");
                            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
                            SetTextColor(hDC, RGB(label.colour.Red, label.colour.Blue, label.colour.Green));
                            SetTextAlign(hDC, TA_CENTER); // Central alignment
                            SetBkMode(hDC, TRANSPARENT); // Transparent background
                            TextOutA(hDC, labelPt.x, labelPt.y, label.label.c_str(), static_cast<int>(label.label.length()));

                            // Cleanup
                            SelectObject(hDC, hOldFont);
                            DeleteObject(hFont);
                        }
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
                std::string acType = rt.acType;
                std::string SID = rt.SID;

                // Check if within inset bounds
                if (acLon < viewArea.minViewLon || acLon > viewArea.maxViewLon || acLat < viewArea.minViewLat || acLat > viewArea.maxViewLat) {
                    continue;
                }

                double acXNorm = (acLon - viewArea.minViewLon) / lonRange;
                double acYNorm = (acLat - viewArea.minViewLat) / latRange;

                POINT acPt = {
                    insetTopLeftPosition.x + static_cast<LONG>(acXNorm * normalInsetWidth),
                    insetTopLeftPosition.y  + normalInsetHeight - static_cast<LONG>(acYNorm * normalInsetHeight)
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
                SetTextAlign(hDC, TA_LEFT); // Left align
                SetBkMode(hDC, TRANSPARENT); // Transparent background
                TextOutA(hDC, acPt.x + 5, acPt.y - 8, acCallsign.c_str(), static_cast<int>(acCallsign.length()));

                // Draw AC Type and SID below callsign if needed
                if (isDataLineShown()) {
                    std::string dataLine = acType + " " + SID;
                    TextOutA(hDC, acPt.x + 5, acPt.y + 6, dataLine.c_str(), static_cast<int>(dataLine.length()));
                }

                // Cleanup
                SelectObject(hDC, hOldFont);
                DeleteObject(hFont);

                SelectObject(hDC, oldBrush);
                DeleteObject(hBrush);

                SelectObject(hDC, oldPen);
                DeleteObject(hPen);
            }

//            if (plugin) plugin->LogEvent("OnRefresh end");
            
            // Used to draw buttons in top-right corner of inset
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0)); // Black color
            HPEN oldPen = (HPEN)SelectObject(hDC, hPen);
            int buttonSize = 10;
            
            // Draw 'Hide' button
            RECT hideButtonRect = {
                insetRect.right,
                insetRect.top,
                insetRect.right - buttonSize,
                insetRect.top + buttonSize
            };
            Rectangle(hDC, hideButtonRect.left, hideButtonRect.top, hideButtonRect.right, hideButtonRect.bottom);
            MoveToEx(hDC, hideButtonRect.left, hideButtonRect.top, NULL);
            LineTo(hDC, hideButtonRect.right, hideButtonRect.bottom);
            MoveToEx(hDC, hideButtonRect.right, hideButtonRect.top, NULL);
            LineTo(hDC, hideButtonRect.left, hideButtonRect.bottom);
            // Register clickable area for 'Hide' button
            RECT ClickableHideArea = {
                hideButtonRect.right,
                hideButtonRect.top,
                hideButtonRect.left,
                hideButtonRect.bottom
            };
            AddScreenObject(HIDEINSETSMRBUTTON, "HIDE_BUTTON", ClickableHideArea, false, "");
            
            // Draw 'Minimise' button
            RECT minimiseButtonRect = {
                hideButtonRect.right,
                insetRect.top,
                hideButtonRect.right - buttonSize,
                insetRect.top + buttonSize
            };
            Rectangle(hDC, minimiseButtonRect.left, minimiseButtonRect.top, minimiseButtonRect.right, minimiseButtonRect.bottom);
            if (!isInsetSMRMinimised()) {
                // Undescore symbol at bottom of button
                MoveToEx(hDC, minimiseButtonRect.left - 3, minimiseButtonRect.bottom - 3, NULL);
                LineTo(hDC, minimiseButtonRect.right + 3, minimiseButtonRect.bottom - 3);
            } else {
                // Box symbol in button
                MoveToEx(hDC, minimiseButtonRect.left - 3, minimiseButtonRect.bottom - 3, NULL);
                LineTo(hDC, minimiseButtonRect.right + 3, minimiseButtonRect.bottom - 3);
                LineTo(hDC, minimiseButtonRect.right + 3, minimiseButtonRect.top + 3);
                LineTo(hDC, minimiseButtonRect.left - 3, minimiseButtonRect.top + 3);
                LineTo(hDC, minimiseButtonRect.left - 3, minimiseButtonRect.bottom - 3);
            }
            // Register clickable area for 'Minimise' button
            RECT ClickableMinimiseArea = {
                minimiseButtonRect.right,
                minimiseButtonRect.top,
                minimiseButtonRect.left,
                minimiseButtonRect.bottom
            };
            AddScreenObject(MINIMISEINSETSMRBUTTON, "MINIMISE_BUTTON", ClickableMinimiseArea, false, "");

            // Cleanup
            SelectObject(hDC, oldPen);
            DeleteObject(hPen);

            // Draw 'InsetSMR' label at top-left of inset
            HFONT hFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                      ANSI_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS,
                                      DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE,
                                      "Arial");
            HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);
            SetTextColor(hDC, RGB(255, 255, 255)); // White color
            SetTextAlign(hDC, TA_LEFT); // Left align
            SetBkMode(hDC, TRANSPARENT); // Transparent background
            const char* label = "InsetSMR";
            TextOutA(hDC, insetRect.left + 5, insetRect.top + 5, label, static_cast<int>(strlen(label)));
            SelectObject(hDC, hOldFont);
            DeleteObject(hFont);

            // Clear clip region
            RestoreDC(hDC, savedDC);
            DeleteObject(clipRgn);
        }
    }

    bool RadarScreen::isShowingInsetSMR() {
        std::lock_guard<std::mutex> lock(showingInsetSMRMutex);
        return showingInsetSMR;
    }

    bool RadarScreen::SetShowingInsetSMR(bool newState) {
        std::lock_guard<std::mutex> lock(showingInsetSMRMutex);
        bool existingState = showingInsetSMR;

        if (newState != existingState) {
            showingInsetSMR = newState;
            return true; // State changed
        }

        return false; // No change
    }

    void RadarScreen::SetInsetViewArea(InsetSMRNS::ViewCoordinates viewArea) {
        std::lock_guard<std::mutex> lock(insetViewAreaMutex);
        insetViewArea = viewArea;
        if (plugin) {
            plugin->LogEvent("insetViewArea set with values: " + std::to_string(insetViewArea.maxViewLat) + " " + std::to_string(insetViewArea.minViewLat) + " " + std::to_string(insetViewArea.maxViewLon) + " " + std::to_string(insetViewArea.minViewLon));
        }
    }

    void RadarScreen::SetScaleFactor(int newScaleFactor) {
        std::lock_guard<std::mutex> lock(scaleFactorMutex);
        scaleFactor = newScaleFactor;
    }

    bool RadarScreen::ToggleDataLine() {
        std::lock_guard<std::mutex> lock (showDataLineMutex);
        showDataLine = !showDataLine;
        return showDataLine;
    }

    bool RadarScreen::ToggleLabels() {
        std::lock_guard<std::mutex> lock (showLabelsMutex);
        showLabels = !showLabels;
        return showLabels;
    }

    InsetSMRNS::ViewCoordinates RadarScreen::getInsetViewArea() {
        std::lock_guard<std::mutex> lock(insetViewAreaMutex);
        InsetSMRNS::ViewCoordinates snapshot = insetViewArea;
        return snapshot;
    }

    void RadarScreen::ToggleInsetSMRMinimised () {
        std::lock_guard<std::mutex> lock(insetSMRMinimisedMutex);
        insetSMRMinimised = !insetSMRMinimised;
    }
    
    bool RadarScreen::isInsetSMRMinimised() {
        std::lock_guard<std::mutex> lock(showingInsetSMRMutex);
        return insetSMRMinimised;
    }

    bool RadarScreen::isDataLineShown() {
        std::lock_guard<std::mutex> lock (showDataLineMutex);
        return showDataLine;
    }

    bool RadarScreen::areLabelsShown() {
        std::lock_guard<std::mutex> lock (showLabelsMutex);
        return showLabels;
    }

    int RadarScreen::getScaleFactor() {
        std::lock_guard<std::mutex> lock(scaleFactorMutex);
        int snapshot = scaleFactor;
        return snapshot;
    }
}