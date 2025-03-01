#pragma once

#include <set>
#include <string>

#include "EuroScope/EuroScopePlugIn.h"

class RadarScreen : public EuroScopePlugIn::CRadarScreen
{
public:
	RadarScreen();
	virtual ~RadarScreen();

	bool groundOnline;
	bool towerOnline;
	bool approachOnline;
	bool eastCenterOnline;
	double eastCenterFreq;
	bool northCenterOnline;
	double northCenterFreq;
	bool centerOnline;
	double centerFreq;
	bool centralCenterOnline;
	double centralCenterFreq;

	inline void OnAsrContentToBeClosed() { delete this; }
	void OnControllerPositionUpdate(EuroScopePlugIn::CController Controller);
	void OnControllerDisconnect(EuroScopePlugIn::CController Controller);

private:
	std::set<std::string> groundStations;
	std::set<std::string> towerStations;
	std::set<std::string> approachStations;
	std::set<std::string> eastCenterStations;
	std::set<std::string> northCenterStations;
	std::set<std::string> centerStations;
	std::set<std::string> centralCenterStations;
};