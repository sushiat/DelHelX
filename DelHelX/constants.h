#pragma once

#include <regex>

#define PLUGIN_NAME    "DelHelX"
#define PLUGIN_VERSION "0.1.0"
#define PLUGIN_AUTHOR  "Markus Korbel"
#define PLUGIN_LICENSE "(c) 2025, MIT License"
#define PLUGIN_LATEST_VERSION_URL "https://raw.githubusercontent.com/sushiat/DelHelX/master/version.txt"
#define PLUGIN_LATEST_DOWNLOAD_URL "https://github.com/sushiat/DelHelX/releases/latest"

const char SETTINGS_DELIMITER = '|';

const int TAG_ITEM_PS_HELPER = 1;
const int TAG_ITEM_TAXIOUT = 2;
const int TAG_ITEM_NEWQNH = 3;

const int TAG_FUNC_ON_FREQ = 100;
const int TAG_FUNC_CLEAR_NEWQNH = 101;

const COLORREF TAG_COLOR_NONE = 0;
const COLORREF TAG_COLOR_RED = RGB(200, 0, 0);
const COLORREF TAG_COLOR_ORANGE = RGB(255, 165, 0);
const COLORREF TAG_COLOR_GREEN = RGB(0, 200, 0);

constexpr auto TOPSKY_PLUGIN_NAME = "TopSky plugin";
const int TOPSKY_TAG_FUNC_SET_CLEARANCE_FLAG = 667;
