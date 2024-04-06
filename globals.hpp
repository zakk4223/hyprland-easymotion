#pragma once
#include <list>

#include <hyprland/src/plugins/PluginAPI.hpp>

inline HANDLE PHANDLE = nullptr;

class CHyprEasyLabel;

struct SGlobalState {
    std::vector<CHyprEasyLabel*>   motionLabels;
};

struct SMotionActionDesc {
  int textSize = 15;
	CColor textColor = CColor(0,0,0,1);
	CColor backgroundColor = CColor(1,1,1,1);
	std::string textFont = "Sans";
	std::string commandString = "";
	CCssGapData boxPadding = CCssGapData();	
	int borderSize = 0;
	CGradientValueData borderColor = CGradientValueData();
	int rounding = 0;
	std::string motionKeys = "abcdefghijklmnopqrstuvwxyz1234567890";
};

inline std::unique_ptr<SGlobalState> g_pGlobalState;

