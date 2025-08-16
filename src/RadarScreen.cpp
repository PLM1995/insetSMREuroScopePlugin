#include "RadarScreen.h"
#include "PLM1995forEuroScope.h"
#include "HelperFunctions.h"
#include "Constants.h"

namespace RadarScreenNS{
    RadarScreen::RadarScreen(PLM1995forEuroScopeNS::PLM1995forEuroScope* pluginInstance) : plugin(pluginInstance) {}

    void RadarScreen::CirclePoint(HDC hDC, POINT point, int radius) {
        Ellipse(hDC, point.x - radius, point.y - radius, point.x + radius, point.y + radius);
    }

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

    void RadarScreen::OnClickScreenObject(int ObjectType, const char * sObjectId, POINT Pt, RECT Area, int Button)
    {
        if (Button == EuroScopePlugIn::BUTTON_LEFT) {
            plugin->DisplayMessage(std::string(sObjectId), "Screen Object Clicked"); // Debugging printout
            switch (ObjectType) {
                case SETPUSHBACKDIRECTIONSTRAIGHT: {
                    for(const auto& [callsign, PushBackData] : plugin->GetPushingBackAircraft()) {
                        if (std::string(sObjectId) == std::string(callsign)) {
                            plugin->SetPushingBackDirection(callsign, PushbackDirection::Straight);
                        }
                    }
                    break;
                }
                case SETPUSHBACKDIRECTIONLEFT: {
                    for(const auto& [callsign, PushBackData] : plugin->GetPushingBackAircraft()) {
                        if (std::string(sObjectId) == std::string(callsign)) {
                            plugin->SetPushingBackDirection(callsign, PushbackDirection::Left);
                        }
                    }
                    break;
                }
                case SETPUSHBACKDIRECTIONRIGHT: {
                    for(const auto& [callsign, PushBackData] : plugin->GetPushingBackAircraft()) {
                        if (std::string(sObjectId) == std::string(callsign)) {
                            plugin->SetPushingBackDirection(callsign, PushbackDirection::Right);
                        }
                    }
                    break;
                }
            }
            
        }
    }
    
    void RadarScreen::OnRefresh(HDC hDC, int phase) {
        if(phase == EuroScopePlugIn::REFRESH_PHASE_AFTER_TAGS) {
            // Prepare GDI objects
            HBRUSH hBrushOld = (HBRUSH)SelectObject(hDC, GetStockObject(NULL_BRUSH)); // No fill

            // Create a pen
            HPEN hPen = CreatePen(PS_SOLID, 1, RGB(255, 165, 0));   // Orange
            HPEN hPenOld = (HPEN)SelectObject(hDC, hPen);

            // For each pushing aircraft
            for(const auto& [callsign, PushBackData] : plugin->GetPushingBackAircraft()) {
                // Convert start position to pixels
                POINT pointStartPush = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(PushBackData.position);
                
                // Draw appropriate T-Bars behind Aicraft
                double reciprocalHeading = fmod(PushBackData.heading + 180.0, 360.0);   // Behind Aircraft
                EuroScopePlugIn::CPosition positionBehind = PositionFromPosHdgDist(PushBackData.position, reciprocalHeading, 50.0);
                POINT pointBehind = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehind);

                double leftHeading = fmod(reciprocalHeading + 90.0, 360.0);
                EuroScopePlugIn::CPosition positionBehindLeft = PositionFromPosHdgDist(positionBehind, leftHeading, 25.0);
                POINT tBarLeftPoint = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehindLeft);

                double rightHeading = fmod(reciprocalHeading + 270.0, 360.0);
                EuroScopePlugIn::CPosition positionBehindRight = PositionFromPosHdgDist(positionBehind, rightHeading, 25.0);
                POINT tBarRightPoint = EuroScopePlugIn::CRadarScreen::ConvertCoordFromPositionToPixel(positionBehindRight);

                switch (PushBackData.direction) {
                    case PushbackDirection::Null: {
                        int circle_radius = 3;
                        CirclePoint(hDC, pointBehind, circle_radius);
                        CirclePoint(hDC, tBarLeftPoint, circle_radius);
                        CirclePoint(hDC, tBarRightPoint, circle_radius);
                        
                        AddScreenObject(SETPUSHBACKDIRECTIONSTRAIGHT, callsign,
                                        { pointBehind.x - circle_radius, pointBehind.y - circle_radius, pointBehind.x + circle_radius, pointBehind.y + circle_radius },
                                        false, "Straight");
                        AddScreenObject(SETPUSHBACKDIRECTIONLEFT, callsign,
                                        { tBarLeftPoint.x - circle_radius, tBarLeftPoint.y - circle_radius, tBarLeftPoint.x + circle_radius, tBarLeftPoint.y + circle_radius },
                                        false, "Left");
                        AddScreenObject(SETPUSHBACKDIRECTIONRIGHT, callsign,
                                        { tBarRightPoint.x - circle_radius, tBarRightPoint.y - circle_radius,tBarRightPoint.x + circle_radius, tBarRightPoint.y + circle_radius },
                                        false, "Right");
                        break;
                    }
                    case PushbackDirection::Left: {
                        MoveToEx(hDC, pointStartPush.x, pointStartPush.y, nullptr);
                        LineTo(hDC, pointBehind.x, pointBehind.y);
                        LineTo(hDC, tBarLeftPoint.x, tBarLeftPoint.y);
                        break;
                    }
                    case PushbackDirection::Right: {
                        MoveToEx(hDC, pointStartPush.x, pointStartPush.y, nullptr);
                        LineTo(hDC, pointBehind.x, pointBehind.y);
                        LineTo(hDC, tBarRightPoint.x, tBarRightPoint.y);
                        break;
                    }
                    case PushbackDirection::Straight: {
                        MoveToEx(hDC, pointStartPush.x, pointStartPush.y, nullptr);
                        LineTo(hDC, pointBehind.x, pointBehind.y);
                        break;
                    }
                }
            }

            // Cleanup GDI objects
            SelectObject(hDC, hPenOld);
            DeleteObject(hPen);
            SelectObject(hDC, hBrushOld);
        }
    }
}