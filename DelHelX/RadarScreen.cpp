#include "pch.h"
#include "RadarScreen.h"

#include <algorithm>
#include <string>
#include "constants.h"

RadarScreen::RadarScreen() : EuroScopePlugIn::CRadarScreen()
{
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
		if (Controller.GetFacility() == 3)
		{
			if (this->groundStations.find(cs) == this->groundStations.end())
			{
				this->groundStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Ground", (cs + "|Ground online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 4 && cs.find("ATIS") == std::string::npos)
		{
			if (this->towerStations.find(cs) == this->towerStations.end())
			{
				this->towerStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Tower", (cs + "|Tower online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 5)
		{
			if (this->approachStations.find(cs) == this->approachStations.end())
			{
				this->approachStations.insert(cs);
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Approach", (cs + "|Approach online").c_str(), true, true, true, false, false);
			}
		}

		if (Controller.GetFacility() == 6)
		{
			if (this->centerStations.find(cs) == this->centerStations.end())
			{
				double freq = Controller.GetPrimaryFrequency();
				this->centerStations.emplace(cs, std::to_string(freq));
				this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|Center online").c_str(), true, true, true, false, false);
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
		if (Controller.GetFacility() == 3)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Ground", (cs + "|Ground disconnected").c_str(), true, true, true, false, false);

			if (this->groundStations.find(cs) != this->groundStations.end())
			{
				this->groundStations.erase(cs);
			}
		}

		if (Controller.GetFacility() == 4 && cs.find("ATIS") == std::string::npos)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Tower", (cs + "|Tower disconnected").c_str(), true, true, true, false, false);

			if (this->towerStations.find(cs) != this->towerStations.end())
			{
				this->towerStations.erase(cs);
			}
		}

		if (Controller.GetFacility() == 5)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Approach", (cs + "|Approach disconnected").c_str(), true, true, true, false, false);

			if (this->approachStations.find(cs) != this->approachStations.end())
			{
				this->approachStations.erase(cs);
			}
		}

		if (Controller.GetFacility() == 6)
		{
			this->GetPlugIn()->DisplayUserMessage(PLUGIN_NAME, "Center", (cs + "|Center disconnected").c_str(), true, true, true, false, false);

			if (this->centerStations.find(cs) != this->centerStations.end())
			{
				this->centerStations.erase(cs);
			}
		}
	}
}

