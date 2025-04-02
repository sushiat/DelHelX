#pragma once

#include <string>
#include <map>
#include <vector>

struct geoGndFreq
{
	std::string name;
	std::string freq;
	std::vector<double> lat;
	std::vector<double> lon;
};

struct taxiOutStands
{
	std::string name;
	std::vector<double> lat;
	std::vector<double> lon;
};

struct napReminder
{
	bool enabled;
	int hour;
	int minute;
	std::string tzone;
	bool triggered;
};

struct airport
{
	std::string icao;
	std::string gndFreq;
	std::string twrFreq;
	std::string appFreq;

	std::map<std::string, geoGndFreq> geoGndFreq;
	std::map<std::string, std::string> rwyTwrFreq;
	std::vector<std::string> ctrStations;
	std::map<std::string, taxiOutStands> taxiOutStands;
	napReminder nap_reminder;
};