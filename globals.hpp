#pragma once
#include <list>

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;

class CHyprEasyLabel;

struct SGlobalState {
	std::vector<WP<CHyprEasyLabel>>   motionLabels;
};

struct SMotionActionDesc {
	int textSize = 15;
	CHyprColor textColor = CHyprColor(0,0,0,1);
	CHyprColor backgroundColor = CHyprColor(1,1,1,1);
	std::string textFont = "Sans";
	std::string commandString = "";
	Config::CCssGapData boxPadding = Config::CCssGapData();	
	int borderSize = 0;
	Config::CGradientValueData borderColor = Config::CGradientValueData();
	int rounding = 0;
	bool blur = false;
	bool xray = false;
	float blurA = 1.0f;
	std::string motionKeys = "abcdefghijklmnopqrstuvwxyz1234567890";
	std::string motionLabels = "";
	std::string fullscreen_action = "none";
	bool only_special = true;
};

inline UP<SGlobalState> g_pGlobalState;

