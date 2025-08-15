#include "RadarScreen.h"
#include "PLM1995forEuroScope.h"

namespace RadarScreenNS{
    RadarScreen::RadarScreen(PLM1995forEuroScopeNS::PLM1995forEuroScope* pluginInstance) : plugin(pluginInstance) {}

    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
            // Prepare GDI objects
            HBRUSH hBrushOld = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH)); // no fill

            // Create a red pen for circle outline
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
            HPEN hPenOld = (HPEN)SelectObject(hDC, hPen);

            const int radius = 25;

            // For each pushing aircraft
            for(const auto& [callsign, position] : plugin->GetPushingBackAircraft()) {
                // Convert position to pixels to draw circle around
                POINT point = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(position);

                // Draw ellipse (circle)
                Ellipse(hDC,
                        point.x - radius,
                        point.y - radius,
                        point.x + radius,
                        point.y + radius);
            }

            // Cleanup GDI objects
            SelectObject(hDC, hPenOld);
            DeleteObject(hPen);
            SelectObject(hDC, hBrushOld);
        }
    }
}