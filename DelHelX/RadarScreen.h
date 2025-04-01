#pragma once

#include <map>
#include <set>
#include <string>

#include "EuroScope/EuroScopePlugIn.h"

class RadarScreen : public EuroScopePlugIn::CRadarScreen
{
public:
	RadarScreen();
	virtual ~RadarScreen();

	bool debug;
	std::set<std::string> groundStations;
	std::set<std::string> towerStations;
	std::set<std::string> approachStations;
	std::map<std::string, std::string> centerStations;

	inline void OnAsrContentToBeClosed() { delete this; }
	void OnControllerPositionUpdate(EuroScopePlugIn::CController Controller);
	void OnControllerDisconnect(EuroScopePlugIn::CController Controller);
};