#include "pch.h"
#include "RadarScreen.h"

#include <algorithm>
#include <string>
#include "constants.h"

RadarScreen::RadarScreen() : EuroScopePlugIn::CRadarScreen()
{
	this->groundOnline = false;
	this->towerOnline = false;
	this->approachOnline = false;
	this->centerOnline = false;
	this->centerFreq = 0;
}

RadarScreen::~RadarScreen()
{
}

void RadarScreen::OnControllerPositionUpdate(EuroScopePlugIn::CController Controller)
{
	std::string cs = Controller.GetCallsign();
	std::transform(cs.begin(), cs.end(), cs.begin(), ::toupper);

	std::string myCS = this->GetPlugIn()->ControllerMyself().GetCallsign();
	std::transform(myCS.begin(), myCS.end(), myCS.begin(), ::toupper);

	// Not interested in observers, non-controllers and my own callsign
	if (Controller.IsController() && Controller.GetRating() > 1 && cs != myCS)
	{
		if (Controller.GetFacility() == 3 && cs.find("LOWW") != std::string::npos)
		{
			this->groundOnline = true;
			if (this->groundStations.find(cs) == this->groundStations.end())
			{
				this->groundStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Ground", (cs + "|Ground online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 4 && cs.find("LOWW") != std::string::npos && cs.find("ATIS") == std::string::npos)
		{
			this->towerOnline = true;
			if (this->towerStations.find(cs) == this->towerStations.end())
			{
				this->towerStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Tower", (cs + "|Tower online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 5 && cs.find("LOWW") != std::string::npos)
		{
			this->approachOnline = true;
			if (this->approachStations.find(cs) == this->approachStations.end())
			{
				this->approachStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Approach", (cs + "|Approach online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_E") != std::string::npos)
		{
			this->eastCenterOnline = true;
			this->eastCenterFreq = Controller.GetPrimaryFrequency();
			if (this->eastCenterStations.find(cs) == this->eastCenterStations.end())
			{
				this->eastCenterStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|E_Center online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_N") != std::string::npos)
		{
			this->northCenterOnline = true;
			this->northCenterFreq = Controller.GetPrimaryFrequency();
			if (this->northCenterStations.find(cs) == this->northCenterStations.end())
			{
				this->northCenterStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|N_Center online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_CTR") != std::string::npos)
		{
			this->centerOnline = true;
			this->centerFreq = Controller.GetPrimaryFrequency();
			if (this->centerStations.find(cs) == this->centerStations.end())
			{
				this->centerStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|Center online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_C_") != std::string::npos)
		{
			this->centralCenterOnline = true;
			this->centralCenterFreq = Controller.GetPrimaryFrequency();
			if (this->centralCenterStations.find(cs) == this->centralCenterStations.end())
			{
				this->centralCenterStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|C_Center online").c_str(), true, true, true, false, false);
			}
		}
	}
}

void RadarScreen::OnControllerDisconnect(EuroScopePlugIn::CController Controller)
{
	std::string cs = Controller.GetCallsign();
	std::transform(cs.begin(), cs.end(), cs.begin(), ::toupper);

	// Not interested in observers and non-controllers
	if (Controller.IsController() && Controller.GetRating() > 1)
	{
		if (Controller.GetFacility() == 3 && cs.find("LOWW") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Ground", (cs + "|Ground disconnected").c_str(), true, true, true, false, false);

			this->groundStations.erase(cs);
			if (this->groundStations.empty())
			{
				this->groundOnline = false;
			}
		}

		if (Controller.GetFacility() == 4 && cs.find("LOWW") != std::string::npos && cs.find("ATIS") == std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Tower", (cs + "|Tower disconnected").c_str(), true, true, true, false, false);

			this->towerStations.erase(cs);
			if (this->towerStations.empty())
			{
				this->towerOnline = false;
			}
		}

		if (Controller.GetFacility() == 5 && cs.find("LOWW") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Approach", (cs + "|Approach disconnected").c_str(), true, true, true, false, false);

			this->approachStations.erase(cs);
			if (this->approachStations.empty())
			{
				this->approachOnline = false;
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_E") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|E_Center disconnected").c_str(), true, true, true, false, false);

			this->eastCenterStations.erase(cs);
			if (this->eastCenterStations.empty())
			{
				this->eastCenterOnline = false;
				this->eastCenterFreq = 0;
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_N") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|N_Center disconnected").c_str(), true, true, true, false, false);

			this->northCenterStations.erase(cs);
			if (this->northCenterStations.empty())
			{
				this->northCenterOnline = false;
				this->northCenterFreq = 0;
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_CTR") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|Center disconnected").c_str(), true, true, true, false, false);

			this->centerStations.erase(cs);
			if (this->centerStations.empty())
			{
				this->centerOnline = false;
				this->centerFreq = 0;
			}
		}

		if (Controller.GetFacility() == 6 && cs.find("LOVV_C_") != std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|C_Center disconnected").c_str(), true, true, true, false, false);

			this->centralCenterStations.erase(cs);
			if (this->centralCenterStations.empty())
			{
				this->centralCenterOnline = false;
				this->centralCenterFreq = 0;
			}
		}
	}
}

