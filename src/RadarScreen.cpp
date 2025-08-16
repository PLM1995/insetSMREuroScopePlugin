#include "RadarScreen.h"
#include "PLM1995forEuroScope.h"
#include "HelperFunctions.h"

namespace RadarScreenNS{
    RadarScreen::RadarScreen(PLM1995forEuroScopeNS::PLM1995forEuroScope* pluginInstance) : plugin(pluginInstance) {}

    EuroScopePlugIn::CPosition RadarScreen::PositionFromPosHdgDist(EuroScopePlugIn::CPosition startPosition, double heading, double distance) {
        // Convert Inputs
        double convertedDistance = (distance * 0.00053996) / 60 * M_PI / 180;
        double headingRad = DegToRad(heading);
		double latStartRad = DegToRad(startPosition.m_Latitude);
		double lonStartRad = DegToRad(startPosition.m_Longitude);

        // Calculate new coordinates
        double new_lat = asin(sin(latStartRad) * cos(convertedDistance) + cos(latStartRad) * sin(convertedDistance) * cos(headingRad));
		double new_lon = cos(new_lat) == 0 ? lonStartRad : fmod(lonStartRad + asin(sin(headingRad) * sin(convertedDistance) / cos(new_lat)) + M_PI, 2 * M_PI) - M_PI;


        // Return the new position
        EuroScopePlugIn::CPosition endPosition;
        endPosition.m_Latitude = RadToDeg(new_lat);
        endPosition.m_Longitude = RadToDeg(new_lon);
        return endPosition;
    }

    
    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
            // Prepare GDI objects
            HBRUSH hBrushOld = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH)); // no fill

            // Create a red pen for circle outline
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
            HPEN hPenOld = (HPEN)SelectObject(hDC, hPen);

            // For each pushing aircraft
            for(const auto& [callsign, PushBackData] : plugin->GetPushingBackAircraft()) {
                // Convert start position to pixels
                POINT pointStartPush = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(PushBackData.position);

                // Draw Line out of back of aircraft
                double reciprocalHeading = fmod(PushBackData.heading + 180, 360);    // Behind aircraft
               
                EuroScopePlugIn::CPosition positionBehind = PositionFromPosHdgDist(PushBackData.position, reciprocalHeading, 50.0);
                POINT pointBehind = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehind);
                MoveToEx(hDC, pointStartPush.x, pointStartPush.y, nullptr);
                LineTo(hDC, pointBehind.x, pointBehind.y);

                // Draw T-Bars behind Aicraft
                double rightHeading = fmod(reciprocalHeading + 270.0, 360.0);
                double leftHeading = fmod(reciprocalHeading + 90.0, 360.0);
                EuroScopePlugIn::CPosition positionBehindRight = PositionFromPosHdgDist(positionBehind, rightHeading, 25.0);
                EuroScopePlugIn::CPosition positionBehindLeft = PositionFromPosHdgDist(positionBehind, leftHeading, 25.0);

                POINT tBarRightPoint = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehindRight);
                POINT tBarLeftPoint = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehindLeft);

                MoveToEx(hDC, pointBehind.x, pointBehind.y, nullptr);
                LineTo(hDC, tBarLeftPoint.x, tBarLeftPoint.y);

                MoveToEx(hDC, pointBehind.x, pointBehind.y, nullptr);
                LineTo(hDC, tBarRightPoint.x, tBarRightPoint.y);
            }

            // Cleanup GDI objects
            SelectObject(hDC, hPenOld);
            DeleteObject(hPen);
            SelectObject(hDC, hBrushOld);
        }
    }
}