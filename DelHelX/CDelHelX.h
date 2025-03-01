#pragma once

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <future>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <psapi.h>

#include "EuroScope/EuroScopePlugIn.h"
#include "semver/semver.hpp"
#include "nlohmann/json.hpp"

#include "constants.h"
#include "helpers.h"
#include "validation.h"
#include "RadarScreen.h"
#include "point.h"

using json = nlohmann::json;
using namespace std::chrono_literals;

class CDelHelX : public EuroScopePlugIn::CPlugIn
{
public:
	CDelHelX();
	virtual ~CDelHelX();

	bool OnCompileCommand(const char* sCommandLine);
	void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize);
	void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area);
	void OnTimer(int Counter);
	EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated);

private:
	bool debug;
	bool updateCheck;
	bool flashOnMessage;
	std::future<std::string> latestVersion;
	RadarScreen* radarScreen;

	void LoadSettings();
	void SaveSettings();

	validation ProcessFlightPlan(EuroScopePlugIn::CFlightPlan& fp, EuroScopePlugIn::CRadarTarget& rt);

	bool PointInsidePolygon(int polyCorners, double polyX[], double polyY[], double x, double y);

	void LogMessage(std::string message);
	void LogMessage(std::string message, std::string handler);
	void LogDebugMessage(std::string message);
	void LogDebugMessage(std::string message, std::string type);

	void CheckForUpdate();
};



