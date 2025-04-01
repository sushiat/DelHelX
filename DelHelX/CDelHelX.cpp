#include "pch.h"

#include "CDelHelX.h"

static CDelHelX* pPlugin;

CDelHelX::CDelHelX() : EuroScopePlugIn::CPlugIn(
	EuroScopePlugIn::COMPATIBILITY_CODE,
	PLUGIN_NAME,
	PLUGIN_VERSION,
	PLUGIN_AUTHOR,
	PLUGIN_LICENSE
)
{
	std::ostringstream msg;
	msg << "Version " << PLUGIN_VERSION << " loaded.";

	this->LogMessage(msg.str(), "Init");

	this->RegisterTagItemType("Push+Start Helper", TAG_ITEM_PS_HELPER);
	this->RegisterTagItemType("Taxi out?", TAG_ITEM_TAXIOUT);
	this->RegisterTagItemFunction("Set ONFREQ/STUP/PUSH", TAG_FUNC_ON_FREQ);
	this->RegisterTagItemType("New QNH", TAG_ITEM_NEWQNH);
	this->RegisterTagItemFunction("Clear new QNH", TAG_FUNC_CLEAR_NEWQNH);

	this->RegisterDisplayType(PLUGIN_NAME, true, false, false, false);

	this->updateCheck = false;
	this->flashOnMessage = false;
	this->groundOverride = false;
	this->towerOverride = false;
	this->noChecks = false;

	this->LoadSettings();
	this->LoadConfig();

	if (this->updateCheck) {
		this->latestVersion = std::async(FetchLatestVersion);
	}
}

CDelHelX::~CDelHelX() = default;

EuroScopePlugIn::CRadarScreen* CDelHelX::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
	this->radarScreen = new RadarScreen();
	this->radarScreen->debug = this->debug;
	return this->radarScreen;
}

bool CDelHelX::OnCompileCommand(const char* sCommandLine)
{
	std::vector<std::string> args = split(sCommandLine);

	if (starts_with(args[0], ".delhelx")) 
	{
		if (args.size() == 1) 
		{
			std::ostringstream msg;
			msg << "Version " << PLUGIN_VERSION << " loaded. Available commands: gnd, twr, nocheck, reset, update, flash, redoflags, testqnh";

			this->LogMessage(msg.str(), "Init");

			return true;
		}

		if (args[1] == "debug") {
			if (this->debug) {
				this->LogMessage("Disabling debug mode", "Debug");
			}
			else {
				this->LogMessage("Enabling debug mode", "Debug");
			}

			this->debug = !this->debug;
			this->radarScreen->debug = this->debug;

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "update") 
		{
			if (this->updateCheck) 
			{
				this->LogMessage("Disabling update check", "Update");
			}
			else {
				this->LogMessage("Enabling update check", "Update");
			}

			this->updateCheck = !this->updateCheck;

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "flash") 
		{
			if (this->flashOnMessage) 
			{
				this->LogMessage("No longer flashing on DelHelX message", "Config");
			}
			else {
				this->LogMessage("Flashing on DelHelX message", "Config");
			}

			this->flashOnMessage = !this->flashOnMessage;

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "gnd")
		{
			if (this->groundOverride)
			{
				this->LogMessage("GND freq override OFF", "GND");
			}
			else {
				this->LogMessage("GND freq override ON", "GND");
			}

			this->groundOverride = !this->groundOverride;

			return true;
		}
		else if (args[1] == "twr")
		{
			if (this->towerOverride)
			{
				this->LogMessage("TWR freq override OFF", "TWR");
			}
			else {
				this->LogMessage("TWR freq override ON", "TWR");
			}

			this->towerOverride = !this->towerOverride;

			return true;
		}
		else if (args[1] == "nocheck")
		{
			if (this->noChecks)
			{
				this->LogMessage("Flight plan checks turned ON", "Checks");
			}
			else {
				this->LogMessage("Flight plan checks turned OFF, use only for testing!!!", "Checks");
			}

			this->noChecks = !this->noChecks;

			return true;
		}
		else if (args[1] == "reset")
		{
			this->LogMessage("Resetting DelHelX plugin to defaults", "Defaults");
			this->updateCheck = false;
			this->flashOnMessage = false;
			this->groundOverride = false;
			this->towerOverride = false;
			this->noChecks = false;

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "redoflags")
		{
			this->LogMessage("Redoing clearance flags...", "Flags");
			this->RedoFlags();

			return true;
		}
		else if (args[1] == "testqnh")
		{
			this->OnNewMetarReceived("LOWW", "LOWW 231805Z 26011KT CAVOK 15/07 Q2000 TEMPO 32015KT");

			return true;
		}
	}

	return false;
}

void CDelHelX::RedoFlags()
{
	for (EuroScopePlugIn::CRadarTarget rt = this->RadarTargetSelectFirst(); rt.IsValid(); rt = this->RadarTargetSelectNext(rt)) {
		EuroScopePlugIn::CRadarTargetPositionData pos = rt.GetPosition();
		// Skip if aircraft is not on the ground (currently using ground speed threshold)
		// TODO better option for finding aircraft on ground??? maybe airport elevation via config???
		if (!pos.IsValid() || pos.GetReportedGS() > 40) {
			continue;
		}

		EuroScopePlugIn::CFlightPlan fp = rt.GetCorrelatedFlightPlan();
		// Skip if aircraft is tracked (with exception of aircraft tracked by current controller)
		if (!fp.IsValid() || (strcmp(fp.GetTrackingControllerId(), "") != 0 && !fp.GetTrackingControllerIsMe())) {
			continue;
		}

		std::string dep = fp.GetFlightPlanData().GetOrigin();
		to_upper(dep);

		std::string arr = fp.GetFlightPlanData().GetDestination();
		to_upper(arr);

		std::string cs = fp.GetCallsign();

		// Skip aircraft without a valid flightplan (no departure/destination airport)
		if (dep.empty() || arr.empty()) {
			continue;
		}

		auto airport = this->airports.find(dep);
		if (airport == this->airports.end())
		{
			// Airport not in config
			return;
		}

		if (fp.GetClearenceFlag())
		{
			// Toggle off and back on
			this->radarScreen->StartTagFunction(cs.c_str(), nullptr, 0, cs.c_str(), nullptr, EuroScopePlugIn::TAG_ITEM_FUNCTION_SET_CLEARED_FLAG, POINT(), RECT());
			this->radarScreen->StartTagFunction(cs.c_str(), nullptr, 0, cs.c_str(), nullptr, EuroScopePlugIn::TAG_ITEM_FUNCTION_SET_CLEARED_FLAG, POINT(), RECT());
		}
	}
}

void CDelHelX::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	if (!FlightPlan.IsValid()) 
	{
		return;
	}
	
	if (ItemCode == TAG_ITEM_PS_HELPER) 
	{
		validation res = this->CheckPushStartStatus(FlightPlan, RadarTarget);

		if (res.valid) 
		{
			strcpy_s(sItemString, 16, res.tag.c_str());
			*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

			if (res.color == TAG_COLOR_NONE) 
			{
				*pRGB = TAG_COLOR_GREEN;
			}
			else {
				*pRGB = res.color;
			}
		}
		else
		{
			strcpy_s(sItemString, 16, res.tag.c_str());

			if (res.color != TAG_COLOR_NONE) 
			{
				*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
				*pRGB = res.color;
			}
		}
	}
	else if (ItemCode == TAG_ITEM_TAXIOUT)
	{
		EuroScopePlugIn::CFlightPlanData fpd = FlightPlan.GetFlightPlanData();
		std::string dep = fpd.GetOrigin();
		to_upper(dep);

		auto airport = this->airports.find(dep);
		if (airport == this->airports.end())
		{
			// Airport not in config
			return;
		}

		EuroScopePlugIn::CPosition position = RadarTarget.GetPosition().GetPosition();

		std::string groundState = FlightPlan.GetGroundState();
		if (groundState.empty() || groundState == "STUP")
		{
			bool isTaxiOut = false;
			for (auto& taxiOut : airport->second.taxiOutStands)
			{
				u_int corners = taxiOut.second.lat.size();
				double lat[10], lon[10];
				std::copy(taxiOut.second.lat.begin(), taxiOut.second.lat.end(), lat);
				std::copy(taxiOut.second.lon.begin(), taxiOut.second.lon.end(), lon);

				if (CDelHelX::PointInsidePolygon(static_cast<int>(corners), lon, lat, position.m_Longitude, position.m_Latitude))
				{
					isTaxiOut = true;
					continue;
				}
			}

			if (isTaxiOut)
			{
				strcpy_s(sItemString, 16, "T");
				*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
				*pRGB = TAG_COLOR_GREEN;
			}
			else
			{
				if (groundState.empty())
				{
					strcpy_s(sItemString, 16, "P");
				}
				else
				{
					strcpy_s(sItemString, 16, "");
				}
			}
		}
		else
		{
			strcpy_s(sItemString, 16, "");
		}
	}
	else if (ItemCode == TAG_ITEM_NEWQNH)
	{
		EuroScopePlugIn::CFlightPlanControllerAssignedData fpcad = FlightPlan.GetControllerAssignedData();
		std::string annotation = fpcad.GetFlightStripAnnotation(2);
		if (annotation=="NQNH")
		{
			strcpy_s(sItemString, 16, "X");
			*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
			*pRGB = TAG_COLOR_ORANGE;
		}
		else
		{
			strcpy_s(sItemString, 16, "");
		}
	}
}

void CDelHelX::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
	EuroScopePlugIn::CFlightPlan fp = this->FlightPlanSelectASEL();
	if (!fp.IsValid()) {
		return;
	}

	EuroScopePlugIn::CFlightPlanData fpd = fp.GetFlightPlanData();
	std::string dep = fpd.GetOrigin();
	to_upper(dep);

	auto airport = this->airports.find(dep);
	if (airport == this->airports.end())
	{
		// Airport not in config
		return;
	}

	EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();

	if (FunctionId == TAG_FUNC_ON_FREQ) {
		validation res = this->CheckPushStartStatus(fp, rt);
		if (res.valid)
		{
			// Are we ground or higher?
			if (this->ControllerMyself().GetFacility() >= 3)
			{
				EuroScopePlugIn::CPosition position = rt.GetPosition().GetPosition();

				bool isTaxiOut = false;
				for (auto& taxiOut : airport->second.taxiOutStands)
				{
					u_int corners = taxiOut.second.lat.size();
					double lat[10], lon[10];
					std::copy(taxiOut.second.lat.begin(), taxiOut.second.lat.end(), lat);
					std::copy(taxiOut.second.lon.begin(), taxiOut.second.lon.end(), lon);

					if (CDelHelX::PointInsidePolygon(static_cast<int>(corners), lon, lat, position.m_Longitude, position.m_Latitude))
					{
						isTaxiOut = true;
						continue;
					}
				}

				if (isTaxiOut)
				{
					std::string scratchBackup(fp.GetControllerAssignedData().GetScratchPadString());
					fp.GetControllerAssignedData().SetScratchPadString("ST-UP");
					fp.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
				}
				else
				{
					// We could give PUSH here, but it's better to visually check, so let's just "pop" it up using ONFREQ
					std::string scratchBackup(fp.GetControllerAssignedData().GetScratchPadString());
					fp.GetControllerAssignedData().SetScratchPadString("ONFREQ");
					fp.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
				}
			}
			else
			{
				// We are delivery set ONFREQ
				std::string scratchBackup(fp.GetControllerAssignedData().GetScratchPadString());
				fp.GetControllerAssignedData().SetScratchPadString("ONFREQ");
				fp.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
			}
		}
	}
	else if (FunctionId == TAG_FUNC_CLEAR_NEWQNH)
	{
		EuroScopePlugIn::CFlightPlanControllerAssignedData fpcad = fp.GetControllerAssignedData();
		fpcad.SetFlightStripAnnotation(2, "");
	}
}

validation CDelHelX::CheckPushStartStatus(EuroScopePlugIn::CFlightPlan& fp, EuroScopePlugIn::CRadarTarget& rt)
{
	validation res{
		true, // valid
		"", // tag
		TAG_COLOR_NONE // color
	};

	std::string cs = fp.GetCallsign();

	std::string groundState = fp.GetGroundState();
	if (!groundState.empty())
	{
		return res;
	}

	EuroScopePlugIn::CFlightPlanData fpd = fp.GetFlightPlanData();
	std::string dep = fpd.GetOrigin();
	to_upper(dep);

	auto airport = this->airports.find(dep);
	if (airport == this->airports.end())
	{
		// Airport not in config, so ignore it
		return res;
	}

	std::string rwy = fpd.GetDepartureRwy();
	if (!this->noChecks && rwy.empty())
	{
		res.valid = false;
		res.tag = "!RWY";
		res.color = TAG_COLOR_RED;

		return res;
	}

	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
	std::string assignedSquawk = cad.GetSquawk();
	std::string currentSquawk = rt.GetPosition().GetSquawk();

	if (this->noChecks && assignedSquawk.empty())
	{
		assignedSquawk = "2000";
	}

	if (assignedSquawk.empty())
	{
		res.valid = false;
		res.tag = "!ASSR";
		res.color = TAG_COLOR_RED;

		return res;
	}

	bool clearanceFlag = fp.GetClearenceFlag();
	if (!this->noChecks && !clearanceFlag)
	{
		res.valid = false;
		res.tag = "!CLR";
		res.color = TAG_COLOR_RED;

		return res;
	}

	if (assignedSquawk != currentSquawk)
	{
		res.tag = assignedSquawk;
		res.color = TAG_COLOR_ORANGE;
	}

	EuroScopePlugIn::CController me = this->ControllerMyself();
	if (me.IsController() && me.GetRating()>1 && me.GetFacility() >= 3)
	{
		if (res.tag.empty())
		{
			res.tag = "OK";
		} else
		{
			res.tag += "->OK";
		}

		return res;
	}

	bool groundOnline = false;
	for (auto station : this->radarScreen->groundStations)
	{
		if (station.find(dep) != std::string::npos)
		{
			groundOnline = true;
			continue;
		}
	}

	if (groundOnline || this->groundOverride)
	{
		EuroScopePlugIn::CPosition position = rt.GetPosition().GetPosition();
		for (auto& geoGnd : airport->second.geoGndFreq)
		{
			u_int corners = geoGnd.second.lat.size();
			double lat[10], lon[10];
			std::copy(geoGnd.second.lat.begin(), geoGnd.second.lat.end(), lat);
			std::copy(geoGnd.second.lon.begin(), geoGnd.second.lon.end(), lon);

			if (CDelHelX::PointInsidePolygon(static_cast<int>(corners), lon, lat, position.m_Longitude, position.m_Latitude))
			{
				res.tag += "->" + geoGnd.second.freq;
				return res;
			}
		}

		// Didn't find any geo-based GND, so return default
		res.tag += "->" + airport->second.gndFreq;
		return res;
	}

	bool towerOnline = false;
	for (auto station : this->radarScreen->towerStations)
	{
		if (station.find(dep) != std::string::npos)
		{
			towerOnline = true;
			continue;
		}
	}

	if (towerOnline || this->towerOverride)
	{
		for (auto rwyFreq : airport->second.rwyTwrFreq)
		{
			if (rwy == rwyFreq.first)
			{
				res.tag += "->" + rwyFreq.second;
				return res;
			}
		}

		// Didn't find a runway specific tower, so return default
		res.tag += "->" + airport->second.twrFreq;
		return res;
	}

	for (auto station : this->radarScreen->approachStations)
	{
		if (station.find(dep) != std::string::npos)
		{
			res.tag += "->" + airport->second.appFreq;
			return res;
		}
	}

	for (auto center : airport->second.ctrStations)
	{
		for (auto station : this->radarScreen->centerStations)
		{
			if (station.first.find(center) != std::string::npos)
			{
				res.tag += "->" + station.second;
				return res;
			}
		}
	}

	// Nothing online, UNICOM
	res.tag += "->122.8";
	return res;
}

bool CDelHelX::PointInsidePolygon(int polyCorners, double polyX[], double polyY[], double x, double y) {
	int   i, j = polyCorners - 1;
	bool  oddNodes = false;

	for (i = 0; i < polyCorners; i++) {
		if (polyY[i] < y && polyY[j] >= y
			|| polyY[j] < y && polyY[i] >= y) {
			if (polyX[i] + (y - polyY[i]) / (polyY[j] - polyY[i]) * (polyX[j] - polyX[i]) < x) {
				oddNodes = !oddNodes;
			}
		}
		j = i;
	}

	return oddNodes;
}

void CDelHelX::LoadSettings()
{
	const char* settings = this->GetDataFromSettings(PLUGIN_NAME);
	if (settings) {
		std::vector<std::string> splitSettings = split(settings, SETTINGS_DELIMITER);

		if (splitSettings.size() < 3) {
			this->LogMessage("Invalid saved settings found, reverting to default.", "Settings");

			this->SaveSettings();

			return;
		}

		std::istringstream(splitSettings[0]) >> this->updateCheck;
		std::istringstream(splitSettings[1]) >> this->flashOnMessage;
		std::istringstream(splitSettings[2]) >> this->debug;

		this->LogMessage("Successfully loaded settings.", "Settings");
	}
	else {
		this->LogMessage("No saved settings found, using defaults.", "Settings");
	}
}

void CDelHelX::SaveSettings()
{
	std::ostringstream ss;
	ss << this->updateCheck << SETTINGS_DELIMITER
		<< this->flashOnMessage << SETTINGS_DELIMITER
	    << this->debug;

	this->SaveDataToSettings(PLUGIN_NAME, "DelHelX settings", ss.str().c_str());
}

void CDelHelX::LoadConfig()
{
	json config;
	try 
	{
		std::filesystem::path base(GetPluginDirectory());
		base.append("config.json");

		std::ifstream ifs(base.c_str());

		config = json::parse(ifs);
	}
	catch (std::exception e)
	{
		this->LogMessage("Failed to read config. Error: " + std::string(e.what()), "Config");
		return;
	}

	for (auto& [icao, json_airport] : config.items())
	{
		// Get basic airport attributes
		airport ap {
			icao,
			json_airport.value<std::string>("gndFreq", ""),
			json_airport.value<std::string>("twrFreq", ""),
			json_airport.value<std::string>("appFreq", "")
		};

		auto ctrStations{ json_airport["ctrStations"].get<std::vector<std::string>>() };
		ap.ctrStations = ctrStations;

		json json_geoGnds;
		try 
		{
			json_geoGnds = json_airport.at("geoGndFreq");
		}
		catch (std::exception e)
		{
			this->LogMessage("Failed to get geographic ground frequencies for airport \"" + icao + "\". Error: " + std::string(e.what()), "Config");
			continue;
		}

		for (auto& [name, json_geoGnd] : json_geoGnds.items())
		{
			geoGndFreq ggf {
				name,
				json_geoGnd.value<std::string>("freq", "")
			};

			auto lat{ json_geoGnd["lat"].get<std::vector<double>>() };
			auto lon{ json_geoGnd["lon"].get<std::vector<double>>() };
			ggf.lat = lat;
			ggf.lon = lon;

			ap.geoGndFreq.emplace(name, ggf);
		}

		json json_rwyTwrs;
		try 
		{
			json_rwyTwrs = json_airport.at("rwyTwrFreq");
		}
		catch (std::exception e)
		{
			this->LogMessage("Failed to get runway tower frequencies for airport \"" + icao + "\". Error: " + std::string(e.what()), "Config");
			continue;
		}

		for (auto& [rwyid, json_rwy] : json_rwyTwrs.items())
		{
			auto rwyFreq = json_rwy.value<std::string>("freq", "");
			ap.rwyTwrFreq.emplace(rwyid, rwyFreq);
		}

		json json_taxiouts;
		try 
		{
			json_taxiouts = json_airport.at("taxiOutStands");
		}
		catch (std::exception e)
		{
			this->LogMessage("Failed to get taxi out stands for airport \"" + icao + "\". Error: " + std::string(e.what()), "Config");
			continue;
		}

		for (auto& [name, json_taxiout] : json_taxiouts.items())
		{
			taxiOutStands tos {
				name
			};

			auto lat{ json_taxiout["lat"].get<std::vector<double>>() };
			auto lon{ json_taxiout["lon"].get<std::vector<double>>() };
			tos.lat = lat;
			tos.lon = lon;

			ap.taxiOutStands.emplace(name, tos);
		}

		this->airports.emplace(icao, ap);
	}

	this->LogMessage("Successfully loaded config for " + std::to_string(this->airports.size()) + " airport(s).", "Config");

	for (auto& airport : this->airports)
	{
		this->LogDebugMessage("Airport: " + airport.first, "Config");
		this->LogDebugMessage("--> GND: " + airport.second.gndFreq, "Config");
		this->LogDebugMessage("--> TWR: " + airport.second.twrFreq, "Config");
		this->LogDebugMessage("--> APP: " + airport.second.appFreq, "Config");
		int ctrIndex = 0;
		for (auto ctr : airport.second.ctrStations)
		{
			this->LogDebugMessage("--> CTR[" + std::to_string(ctrIndex) + "]: " + ctr, "Config");
			ctrIndex++;
		}
		for (auto& geoGnd : airport.second.geoGndFreq)
		{
			this->LogDebugMessage("--> GeoGnd " + geoGnd.first, "Config");
			this->LogDebugMessage("----> FRQ: " + geoGnd.second.freq, "Config");
			std::string lat_string = std::accumulate(std::begin(geoGnd.second.lat), std::end(geoGnd.second.lat), std::string(),
				[](std::string& ss, double s)
				{
					return ss.empty() ? std::to_string(s) : ss + ", " + std::to_string(s);
				});
			this->LogDebugMessage("----> LAT: " + lat_string, "Config");
			std::string lon_string = std::accumulate(std::begin(geoGnd.second.lon), std::end(geoGnd.second.lon), std::string(),
				[](std::string& ss, double s)
				{
					return ss.empty() ? std::to_string(s) : ss + ", " + std::to_string(s);
				});
			this->LogDebugMessage("----> LON: " + lon_string, "Config");
		}
		for (auto& twrRwy : airport.second.rwyTwrFreq)
		{
			this->LogDebugMessage("--> TWR[" + twrRwy.first + "]: " + twrRwy.second, "Config");
		}
		for (auto& taxiOut : airport.second.taxiOutStands)
		{
			this->LogDebugMessage("--> TaxiOut " + taxiOut.first, "Config");
			std::string lat_string = std::accumulate(std::begin(taxiOut.second.lat), std::end(taxiOut.second.lat), std::string(),
				[](std::string& ss, double s)
				{
					return ss.empty() ? std::to_string(s) : ss + ", " + std::to_string(s);
				});
			this->LogDebugMessage("----> LAT: " + lat_string, "Config");
			std::string lon_string = std::accumulate(std::begin(taxiOut.second.lon), std::end(taxiOut.second.lon), std::string(),
				[](std::string& ss, double s)
				{
					return ss.empty() ? std::to_string(s) : ss + ", " + std::to_string(s);
				});
			this->LogDebugMessage("----> LON: " + lon_string, "Config");
		}
	}
	
}

void CDelHelX::LogMessage(const std::string& message, const std::string& type)
{
	this->DisplayUserMessage(PLUGIN_NAME, type.c_str(), message.c_str(), true, true, true, this->flashOnMessage, false);
}


void CDelHelX::LogDebugMessage(const std::string& message, const std::string& type)
{
	if (this->debug) {
		this->LogMessage(message, type);
	}
}

void CDelHelX::OnTimer(int Counter)
{
	if (this->updateCheck && this->latestVersion.valid() && this->latestVersion.wait_for(0ms) == std::future_status::ready) {
		this->CheckForUpdate();
	}
}

void CDelHelX::OnNewMetarReceived(const char* sStation, const char* sFullMetar)
{
	std::string station = sStation;
	to_upper(station);

	this->LogDebugMessage("New METAR for station " + station + ": " + sFullMetar, "Metar");

	auto airport = this->airports.find(station);
	if (airport == this->airports.end())
	{
		// Station not in airport config, so ignore it
		return;
	}

	std::vector<std::string> metarElements = split(sFullMetar);
	for (std::string metarElement : metarElements)
	{
		static const std::regex qnh(R"(Q[0-9]{4})");
		static const std::regex alt(R"(A[0-9]{4})");

		if (std::regex_match(metarElement, qnh) || std::regex_match(metarElement, alt))
		{
			// Check if existing QNH and if that is now different
			auto existingQNH = this->airportQNH.find(station);
			if (existingQNH == this->airportQNH.end())
			{
				this->LogDebugMessage("First QNH value for airport " + station + " is " + metarElement, "Metar");

				// No existing QNH, add it
				this->airportQNH.emplace(station, metarElement);
			}
			else
			{
				if (existingQNH->second != metarElement)
				{
					this->LogDebugMessage("New QNH value for airport " + station + " is " + metarElement, "Metar");

					// Save new QNH
					this->airportQNH[station] = metarElement;

					// Set flight strip annotation on aircraft on the ground at that airport
					for (EuroScopePlugIn::CRadarTarget rt = this->RadarTargetSelectFirst(); rt.IsValid(); rt = this->RadarTargetSelectNext(rt)) {
						EuroScopePlugIn::CRadarTargetPositionData pos = rt.GetPosition();

						// Skip aircraft is not on the ground
						// TODO better option for finding aircraft on ground??? maybe airport elevation via config???
						if (!pos.IsValid() || pos.GetReportedGS() > 40) {
							continue;
						}

						EuroScopePlugIn::CFlightPlan fp = rt.GetCorrelatedFlightPlan();
						// Skip aircraft is tracked (with exception of aircraft tracked by current controller)
						if (!fp.IsValid() || (strcmp(fp.GetTrackingControllerId(), "") != 0 && !fp.GetTrackingControllerIsMe())) {
							continue;
						}

						std::string dep = fp.GetFlightPlanData().GetOrigin();
						to_upper(dep);

						if (dep == station && fp.GetClearenceFlag())
						{
							EuroScopePlugIn::CFlightPlanControllerAssignedData fpcad = fp.GetControllerAssignedData();
							fpcad.SetFlightStripAnnotation(2, "NQNH");
						}
					}
				}
			}
		}
	}
}

void CDelHelX::CheckForUpdate()
{
	try
	{
		semver::version latest{ this->latestVersion.get() };
		semver::version current{ PLUGIN_VERSION };

		if (latest > current) {
			std::ostringstream ss;
			ss << "A new version (" << latest << ") of " << PLUGIN_NAME << " is available, download it at " << PLUGIN_LATEST_DOWNLOAD_URL;

			this->LogMessage(ss.str(), "Update");
		}
	}
	catch (std::exception& e)
	{
		MessageBox(NULL, e.what(), PLUGIN_NAME, MB_OK | MB_ICONERROR);
	}

	this->latestVersion = std::future<std::string>();
}

void __declspec (dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
	*ppPlugInInstance = pPlugin = new CDelHelX();
}

void __declspec (dllexport) EuroScopePlugInExit(void)
{
	delete pPlugin;
}