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
                    auto geoLines = sectorFileLoader->getGeoLines();

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
                    
                    // Draw each GEO line
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

                        HPEN hPen = CreatePen(PS_SOLID, 1,
                            RGB(geoLine.colourRed, geoLine.colourGreen, geoLine.colourBlue));
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