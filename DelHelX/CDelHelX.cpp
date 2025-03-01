#include "pch.h"

#include "CDelHelX.h"

CDelHelX* pPlugin;

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
	this->RegisterTagItemFunction("Set ONFREQ", TAG_FUNC_ON_FREQ);

	this->RegisterDisplayType(PLUGIN_NAME, true, false, false, false);

	this->debug = false;
	this->updateCheck = false;
	this->flashOnMessage = false;

	this->LoadSettings();

	if (this->updateCheck) {
		this->latestVersion = std::async(FetchLatestVersion);
	}
}

CDelHelX::~CDelHelX()
{
}

bool CDelHelX::OnCompileCommand(const char* sCommandLine)
{
	std::vector<std::string> args = split(sCommandLine);

	if (starts_with(args[0], ".delhelx")) {
		if (args.size() == 1) {
			std::ostringstream msg;
			msg << "Version " << PLUGIN_VERSION << " loaded. Available commands: auto, debug, reload, reset, update, flash";

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

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "update") {
			if (this->updateCheck) {
				this->LogMessage("Disabling update check", "Update");
			}
			else {
				this->LogMessage("Enabling update check", "Update");
			}

			this->updateCheck = !this->updateCheck;

			this->SaveSettings();

			return true;
		}
		else if (args[1] == "flash") {
			if (this->flashOnMessage) {
				this->LogMessage("No longer flashing on DelHel message", "Config");
			}
			else {
				this->LogMessage("Flashing on DelHel message", "Config");
			}

			this->flashOnMessage = !this->flashOnMessage;

			this->SaveSettings();

			return true;
		}
	}

	return false;
}

void CDelHelX::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	if (!FlightPlan.IsValid()) {
		return;
	}
	
	switch (ItemCode) {
		case TAG_ITEM_PS_HELPER:
			validation res = this->ProcessFlightPlan(FlightPlan, RadarTarget);

			if (res.valid) {
				if (res.tag.empty()) {
					strcpy_s(sItemString, 16, "??");
				}
				else
				{
					strcpy_s(sItemString, 16, res.tag.c_str());
				}

				*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;

				if (res.color == TAG_COLOR_NONE) {
					*pRGB = TAG_COLOR_GREEN;
				}
				else {
					*pRGB = res.color;
				}
			}
			else
			{
				strcpy_s(sItemString, 16, res.tag.c_str());

				if (res.color != TAG_COLOR_NONE) {
					*pColorCode = EuroScopePlugIn::TAG_COLOR_RGB_DEFINED;
					*pRGB = res.color;
				}
			}
		
			break;

	}
}

void CDelHelX::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
	EuroScopePlugIn::CFlightPlan fp = this->FlightPlanSelectASEL();
	if (!fp.IsValid()) {
		return;
	}

	EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();

	switch (FunctionId) {
		case TAG_FUNC_ON_FREQ:
			validation res = this->ProcessFlightPlan(fp, rt);
			if (res.valid)
			{
				std::string scratchBackup(fp.GetControllerAssignedData().GetScratchPadString());
				fp.GetControllerAssignedData().SetScratchPadString("ONFREQ");
				fp.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
			}

			break;
	}
}

void CDelHelX::OnTimer(int Counter)
{
	if (this->updateCheck && this->latestVersion.valid() && this->latestVersion.wait_for(0ms) == std::future_status::ready) {
		this->CheckForUpdate();
	}
}

EuroScopePlugIn::CRadarScreen* CDelHelX::OnRadarScreenCreated(const char* sDisplayName, bool NeedRadarContent, bool GeoReferenced, bool CanBeSaved, bool CanBeCreated)
{
	this->radarScreen = new RadarScreen();
	return this->radarScreen;
}

void CDelHelX::LoadSettings()
{
	const char* settings = this->GetDataFromSettings(PLUGIN_NAME);
	if (settings) {
		std::vector<std::string> splitSettings = split(settings, SETTINGS_DELIMITER);

		if (splitSettings.size() < 3) {
			this->LogMessage("Invalid saved settings found, reverting to default.");

			this->SaveSettings();

			return;
		}

		std::istringstream(splitSettings[0]) >> this->debug;
		std::istringstream(splitSettings[1]) >> this->updateCheck;
		std::istringstream(splitSettings[2]) >> this->flashOnMessage;

		this->LogDebugMessage("Successfully loaded settings.");
	}
	else {
		this->LogMessage("No saved settings found, using defaults.");
	}
}

void CDelHelX::SaveSettings()
{
	std::ostringstream ss;
	ss << this->debug << SETTINGS_DELIMITER
		<< this->updateCheck << SETTINGS_DELIMITER
		<< this->flashOnMessage;

	this->SaveDataToSettings(PLUGIN_NAME, "DelHelX settings", ss.str().c_str());
}

validation CDelHelX::ProcessFlightPlan(EuroScopePlugIn::CFlightPlan& fp, EuroScopePlugIn::CRadarTarget& rt)
{
	validation res{
		true, // valid
		"", // tag
		TAG_COLOR_NONE // color
	};

	std::string cs = fp.GetCallsign();

	EuroScopePlugIn::CFlightPlanData fpd = fp.GetFlightPlanData();
	std::string dep = fpd.GetOrigin();
	to_upper(dep);

	if (strcmp(dep.c_str(), "LOWW") != 0)
	{
		res.valid = false;
		res.tag = "ADEP";
		res.color = TAG_COLOR_RED;

		return res;
	}

	bool clearanceFlag = fp.GetClearenceFlag();
	if (!clearanceFlag)
	{
		res.valid = false;
		res.tag = "!CLR";
		res.color = TAG_COLOR_RED;

		return res;
	}
	
	std::string rwy = fpd.GetDepartureRwy();
	if (rwy == "") 
	{
		res.valid = false;
		res.tag = "!RWY";
		res.color = TAG_COLOR_RED;

		return res;
	}

	EuroScopePlugIn::CFlightPlanControllerAssignedData cad = fp.GetControllerAssignedData();
	std::string assignedSquawk = cad.GetSquawk();
	std::string currentSquawk = rt.GetPosition().GetSquawk();

	if (assignedSquawk == "")
	{
		res.valid = false;
		res.tag = "!ASSR";
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

	if (this->radarScreen->groundOnline)
	{
		// Bounding box coordinates for 121.775 ground frequency
		// TL 48.129167 16.53675 // GAC top left
		// TR 48.117222 16.570472 // Terminal 2 top right
		// BL 48.123917 16.533667 // HP Rwy 11 bottom left
		// BR 48.113056 16.567444 // HP Rwy 29 (A4) bottom right
		double polyX[] = { 16.533667, 16.567444, 16.570472, 16.53675 };
		double polyY[] = { 48.123917, 48.113056, 48.117222, 48.129167 };
		EuroScopePlugIn::CPosition position = rt.GetPosition().GetPosition();
		if (this->PointInsidePolygon(4, polyX, polyY, position.m_Longitude, position.m_Latitude))
		{
			res.tag += "->121.775";
		}
		else
		{
			res.tag += "->121.6";
		}
	}
	else if (this->radarScreen->towerOnline)
	{
		if (rwy=="29" || rwy =="11")
		{
			res.tag += "->119.4";
		}
		else
		{
			res.tag += "->123.8";
		}
	}
	else if (this->radarScreen->approachOnline)
	{
		res.tag += "->134.675";
	}
	else if (this->radarScreen->eastCenterOnline && this->radarScreen->eastCenterFreq > 0)
	{
		std::ostringstream strs;
		strs << this->radarScreen->eastCenterFreq;
		std::string str = strs.str();

		res.tag += "->" + str;
	}
	else if (this->radarScreen->northCenterOnline && this->radarScreen->northCenterFreq > 0)
	{
		std::ostringstream strs;
		strs << this->radarScreen->northCenterFreq;
		std::string str = strs.str();

		res.tag += "->" + str;
	}
	else if (this->radarScreen->centerOnline && this->radarScreen->centerFreq > 0)
	{
		std::ostringstream strs;
		strs << this->radarScreen->centerFreq;
		std::string str = strs.str();

		res.tag += "->" + str;
	}
	else if (this->radarScreen->centralCenterOnline && this->radarScreen->centralCenterFreq > 0)
	{
		std::ostringstream strs;
		strs << this->radarScreen->centralCenterFreq;
		std::string str = strs.str();

		res.tag += "->" + str;
	}
	else
	{
		res.tag = "->122.8";
	}

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

void CDelHelX::LogMessage(std::string message)
{
	this->DisplayUserMessage("Message", PLUGIN_NAME, message.c_str(), true, true, true, false, false);
}

void CDelHelX::LogMessage(std::string message, std::string type)
{
	this->DisplayUserMessage(PLUGIN_NAME, type.c_str(), message.c_str(), true, true, true, this->flashOnMessage, false);
}

void CDelHelX::LogDebugMessage(std::string message)
{
	if (this->debug) {
		this->LogMessage(message);
	}
}

void CDelHelX::LogDebugMessage(std::string message, std::string type)
{
	if (this->debug) {
		this->LogMessage(message, type);
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