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
	this->RegisterTagItemFunction("Set ONFREQ", TAG_FUNC_ON_FREQ);

	this->RegisterDisplayType(PLUGIN_NAME, true, false, false, false);

	this->updateCheck = false;
	this->flashOnMessage = false;
	this->groundOverride = false;
	this->towerOverride = false;
	this->noChecks = false;

	this->LoadSettings();

	if (this->updateCheck) {
		this->latestVersion = std::async(FetchLatestVersion);
	}
}

CDelHelX::~CDelHelX() = default;

bool CDelHelX::OnCompileCommand(const char* sCommandLine)
{
	std::vector<std::string> args = split(sCommandLine);

	if (starts_with(args[0], ".delhelx")) 
	{
		if (args.size() == 1) 
		{
			std::ostringstream msg;
			msg << "Version " << PLUGIN_VERSION << " loaded. Available commands: gnd, twr, nocheck, reset, update, flash";

			this->LogMessage(msg.str(), "Init");

			return true;
		}

		if (args[1] == "update") 
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
				this->LogMessage("No longer flashing on DelHel message", "Config");
			}
			else {
				this->LogMessage("Flashing on DelHel message", "Config");
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
	}

	return false;
}

void CDelHelX::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan, EuroScopePlugIn::CRadarTarget RadarTarget, int ItemCode, int TagData, char sItemString[16], int* pColorCode, COLORREF* pRGB, double* pFontSize)
{
	if (!FlightPlan.IsValid()) {
		return;
	}
	
	if (ItemCode == TAG_ITEM_PS_HELPER) {
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
	}
}

void CDelHelX::OnFunctionCall(int FunctionId, const char* sItemString, POINT Pt, RECT Area)
{
	EuroScopePlugIn::CFlightPlan fp = this->FlightPlanSelectASEL();
	if (!fp.IsValid()) {
		return;
	}

	EuroScopePlugIn::CRadarTarget rt = fp.GetCorrelatedRadarTarget();

	if (FunctionId == TAG_FUNC_ON_FREQ) {
		validation res = this->ProcessFlightPlan(fp, rt);
		if (res.valid)
		{
			// Are we ground or higher?
			if (this->ControllerMyself().GetFacility() >= 3)
			{
				// TODO can we define polygons for taxi out stands? and issue push if not in it?

				std::string scratchBackup(fp.GetControllerAssignedData().GetScratchPadString());
				fp.GetControllerAssignedData().SetScratchPadString("ST-UP");
				fp.GetControllerAssignedData().SetScratchPadString(scratchBackup.c_str());
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
			this->LogMessage("Invalid saved settings found, reverting to default.", "Settings");

			this->SaveSettings();

			return;
		}

		std::istringstream(splitSettings[0]) >> this->updateCheck;
		std::istringstream(splitSettings[1]) >> this->flashOnMessage;

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
		<< this->flashOnMessage;

	this->SaveDataToSettings(PLUGIN_NAME, "DelHelX settings", ss.str().c_str());
}

validation CDelHelX::ProcessFlightPlan(EuroScopePlugIn::CFlightPlan& fp, EuroScopePlugIn::CRadarTarget& rt) const
{
	validation res{
		true, // valid
		"", // tag
		TAG_COLOR_NONE // color
	};

	std::string groundState = fp.GetGroundState();
	if (!groundState.empty())
	{
		
	}

	std::string cs = fp.GetCallsign();

	EuroScopePlugIn::CFlightPlanData fpd = fp.GetFlightPlanData();
	std::string dep = fpd.GetOrigin();
	to_upper(dep);

	if (!this->noChecks && strcmp(dep.c_str(), "LOWW") != 0)
	{
		res.valid = false;
		res.tag = "ADEP";
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

	if (this->radarScreen->groundOnline || this->groundOverride)
	{
		// Bounding box coordinates for 121.775 ground frequency
		// TL 48.129167 16.53675 // GAC top left
		// TR 48.117222 16.570472 // Terminal 2 top right
		// BL 48.123917 16.533667 // HP Rwy 11 bottom left
		// BR 48.113056 16.567444 // HP Rwy 29 (A4) bottom right
		double polyX[] = { 16.533667, 16.567444, 16.570472, 16.53675 };
		double polyY[] = { 48.123917, 48.113056, 48.117222, 48.129167 };
		EuroScopePlugIn::CPosition position = rt.GetPosition().GetPosition();
		if (CDelHelX::PointInsidePolygon(4, polyX, polyY, position.m_Longitude, position.m_Latitude))
		{
			res.tag += "->121.775";
		}
		else
		{
			res.tag += "->121.6";
		}
	}
	else if (this->radarScreen->towerOnline || this->towerOverride)
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

void CDelHelX::LogMessage(const std::string& message, const std::string& type)
{
	this->DisplayUserMessage(PLUGIN_NAME, type.c_str(), message.c_str(), true, true, true, this->flashOnMessage, false);
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