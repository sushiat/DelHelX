#pragma once

#include <string>
#include <sstream>
#include <future>
#include <fstream>
#include <filesystem>

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

	bool OnCompileCommand(const char* sCommandLine) override;
	void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize) override;
	void OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area) override;
	void OnTimer(int Counter) override;
	EuroScopePlugIn::CRadarScreen* OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated) override;

private:
	bool updateCheck;
	bool flashOnMessage;
	bool groundOverride;
	bool towerOverride;
	bool noChecks;
	std::future<std::string> latestVersion;
	RadarScreen* radarScreen;

	void LoadSettings();
	void SaveSettings();

	validation ProcessFlightPlan(EuroScopePlugIn::CFlightPlan& fp, EuroScopePlugIn::CRadarTarget& rt);
	static bool PointInsidePolygon(int polyCorners, double polyX[], double polyY[], double x, double y);
	void RedoFlags();

	void LogMessage(const std::string& message, const std::string& type);
	void CheckForUpdate();
};



