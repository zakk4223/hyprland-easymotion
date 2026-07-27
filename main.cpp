#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <string>
#include <unistd.h>

#include <any>
#include <ranges>
#include <cmath>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprland/src/managers/fullscreen/FullscreenController.hpp>
#include <hyprland/src/desktop/state/WindowState.hpp>
#include <hyprland/src/state/MonitorState.hpp>
#include <hyprland/src/config/values/ConfigValues.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/config/shared/parserUtils/ParserUtils.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprutils/string/VarList2.hpp>
#include <strings.h>

#include "easymotionDeco.hpp"
#include "globals.hpp"

using namespace Hyprutils::String;

// Plugin config value pointers
inline SP<Config::Values::CIntValue>    g_textSize;
inline SP<Config::Values::CIntValue>    g_textColor;
inline SP<Config::Values::CIntValue>    g_bgColor;
inline SP<Config::Values::CStringValue> g_textFont;
inline SP<Config::Values::CStringValue> g_textPadding;
inline SP<Config::Values::CIntValue>    g_borderSize;
inline SP<Config::Values::CStringValue> g_borderColor;
inline SP<Config::Values::CIntValue>    g_rounding;
inline SP<Config::Values::CIntValue>    g_blur;
inline SP<Config::Values::CIntValue>    g_xray;
inline SP<Config::Values::CFloatValue>  g_blurA;
inline SP<Config::Values::CStringValue> g_motionKeys;
inline SP<Config::Values::CStringValue> g_motionLabels;
inline SP<Config::Values::CStringValue> g_fullscreenAction;
inline SP<Config::Values::CIntValue>    g_onlySpecial;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
	return HYPRLAND_API_VERSION;
}


SDispatchResult easymotionExitDispatch(std::string args)
{
	for (auto &ml : g_pGlobalState->motionLabels | std::ranges::views::reverse) {
		if (ml->m_origFSMode != Fullscreen::controller()->getFullscreenModes(ml->getOwner()).internal)
			Fullscreen::controller()->setFullscreenMode(ml->getOwner(), ml->m_origFSMode);
		ml->getOwner()->removeWindowDeco(ml.get());
	}
	HyprlandAPI::invokeHyprctlCommand("dispatch", "submap reset");
	g_pEventManager->postEvent(SHyprIPCEvent{"easymotionexit", ""});
	return {};

}

SDispatchResult easymotionActionDispatch(std::string args)
{
	for (auto &ml : g_pGlobalState->motionLabels) {
		if (ml->m_szKey == args) {
			g_pEventManager->postEvent(SHyprIPCEvent{"easymotionselect", std::format("{},{}", ml->m_szWindowAddress, ml->m_szKey)});
			g_pKeybindManager->m_dispatchers["exec"](ml->m_szActionCmd);
			easymotionExitDispatch("");
			break;
		}
	}

	return {};
}

void addEasyMotionKeybinds()
{
	g_pKeybindManager->addKeybind(SKeybind{"escape", {}, 0, 0, 0, {}, "easymotionexit", "", 0, "__easymotionsubmap__", "", "", 0, 0, 0, 0, 0, 0, 0, 0});
	g_pKeybindManager->addKeybind(SKeybind{"", {}, 0, 1, 0, {}, "", "", 0, "__easymotionsubmap__", "", "", 0, 0, 0, 0, 0, 0, 0, 0});
}


void addLabelToWindow(PHLWINDOW window, SMotionActionDesc *actionDesc, std::string &key, std::string &label)
{
	UP<CHyprEasyLabel> motionlabel = makeUnique<CHyprEasyLabel>(window, actionDesc);
	motionlabel->m_szKey = key;
	motionlabel->m_szLabel = label;
	g_pGlobalState->motionLabels.emplace_back(motionlabel);
	motionlabel->m_self = motionlabel;
	motionlabel->draw(window->m_monitor.lock(), 1.0);
	motionlabel->m_origFSMode = Fullscreen::controller()->getFullscreenModes(window).internal;
	if ((motionlabel->m_origFSMode != Fullscreen::FSMODE_NONE) && (actionDesc->fullscreen_action != "none"))
	{
		if (actionDesc->fullscreen_action == "maximize")
		{
			Fullscreen::controller()->setFullscreenMode(window, Fullscreen::FSMODE_MAXIMIZED);
		} else if (actionDesc->fullscreen_action == "toggle") {
			Fullscreen::controller()->setFullscreenMode(window, Fullscreen::FSMODE_NONE);
		}
	}
	HyprlandAPI::addWindowDecoration(PHANDLE, window, std::move(motionlabel));
}

static bool parseBorderGradient(std::string VALUE, Config::CGradientValueData *DATA) {
	std::string V = VALUE;

	CVarList   varlist(V, 0, ' ');
	DATA->m_colors.clear();

	std::string parseError = "";

	for (auto& var : varlist) {
		if (var.find("deg") != std::string::npos) {
			try {
				DATA->m_angle = std::stoi(var.substr(0, var.find("deg"))) * (M_PI / 180.0);
			} catch (...) {
        		Log::logger->log(Log::WARN, "Error parsing gradient {}", V);
				return false;
			}

			break;
		}

		if (DATA->m_colors.size() >= 10) {
			Log::logger->log(Log::WARN, "Error parsing gradient {}: max colors is 10.", V);
			return false;
		}

		try {
			DATA->m_colors.push_back(CHyprColor(Config::ParserUtils::parseColor(var).value_or(0)));
		} catch (std::exception& e) {
			Log::logger->log(Log::WARN, "Error parsing gradient {}", V);
		}
	}

	if (DATA->m_colors.size() == 0) {
		Log::logger->log(Log::WARN, "Error parsing gradient {}", V);
		DATA->m_colors.push_back(0);
	}

	DATA->updateColorsOk();
	return true;
}

static int configGetInt(SP<Config::Values::CIntValue> val) {
    return (int)(val->value());
}

static std::string configGetString(SP<Config::Values::CStringValue> val) {
    return val->value();
}

static float configGetFloat(SP<Config::Values::CFloatValue> val) {
    return val->value();
}

SDispatchResult easymotionDispatch(std::string args)
{
	SMotionActionDesc actionDesc;

	actionDesc.textSize = configGetInt(g_textSize);
	actionDesc.textColor = CHyprColor(configGetInt(g_textColor));
	actionDesc.backgroundColor = CHyprColor(configGetInt(g_bgColor));
	actionDesc.textFont = configGetString(g_textFont);
	CVarList2 cpadding = CVarList2(configGetString(g_textPadding));
	actionDesc.boxPadding.parseGapData(cpadding);
	actionDesc.rounding = configGetInt(g_rounding);
	actionDesc.borderSize = configGetInt(g_borderSize);
	if(!parseBorderGradient(configGetString(g_borderColor), &actionDesc.borderColor)) {
		actionDesc.borderColor.m_colors.clear();
		actionDesc.borderColor.m_angle = 0;
	}
	actionDesc.motionKeys = configGetString(g_motionKeys);
	actionDesc.motionLabels = configGetString(g_motionLabels);
	actionDesc.blur = configGetInt(g_blur);
	actionDesc.xray = configGetInt(g_xray);
	actionDesc.blurA = configGetFloat(g_blurA);
	actionDesc.fullscreen_action = std::string(configGetString(g_fullscreenAction));
	actionDesc.only_special = configGetInt(g_onlySpecial);

	CVarList emargs(args, 0, ',');

	for(size_t i = 0; i < emargs.size(); i++)
	{
		CVarList kv(emargs[i], 2, ':');
		if (kv[0] == "action") {
			actionDesc.commandString = kv[1];
		} else if (kv[0] == "textsize") {
			actionDesc.textSize = Config::ParserUtils::parseInt(kv[1]).value_or(15);
		} else if (kv[0] == "textcolor") {
			actionDesc.textColor = CHyprColor(Config::ParserUtils::parseColor(kv[1]).value_or(0xffffffff));
		} else if (kv[0] == "bgcolor") {
			actionDesc.backgroundColor = CHyprColor(Config::ParserUtils::parseColor(kv[1]).value_or(0));
		} else if (kv[0] == "textfont") {
			actionDesc.textFont = kv[1];
		} else if (kv[0] == "textpadding") {
			CVarList2 padVars = CVarList2(kv[1], 0, 's');
			actionDesc.boxPadding.parseGapData(padVars);
		} else if (kv[0] == "rounding") {
			actionDesc.rounding = Config::ParserUtils::parseInt(kv[1]).value_or(0);
		} else if (kv[0] == "bordersize") {
			actionDesc.borderSize = Config::ParserUtils::parseInt(kv[1]).value_or(0);
		} else if (kv[0] == "bordercolor") {
			CVarList varlist(kv[1], 0, 's');
			actionDesc.borderColor.m_colors.clear();
			actionDesc.borderColor.m_angle = 0;
			if(!parseBorderGradient(kv[1], &actionDesc.borderColor)) {
				actionDesc.borderColor.m_colors.clear();
				actionDesc.borderColor.m_angle = 0;
			}
		} else if (kv[0] == "motionkeys") {
			actionDesc.motionKeys = kv[1];
		} else if (kv[0] == "motionlabels") {
			actionDesc.motionLabels = kv[1];
		} else if (kv[0] == "blur") {
			actionDesc.blur = Config::ParserUtils::parseInt(kv[1]).value_or(1);
		} else if (kv[0] == "xray") {
			actionDesc.xray = Config::ParserUtils::parseInt(kv[1]).value_or(1);
		} else if (kv[0] == "blurA") {
			try {
				actionDesc.blurA = std::stof(kv[1]);
			} catch (const std::invalid_argument& ia) {
				actionDesc.blurA = 1.0f;
			}
		} else if (kv[0] == "fullscreen_action") {
			actionDesc.fullscreen_action = kv[1];
		} else if (kv[0] == "only_special") {
			actionDesc.only_special = Config::ParserUtils::parseInt(kv[1]).value_or(1);
		}
	}

	if (actionDesc.motionLabels.size() == 0) {
		actionDesc.motionLabels = actionDesc.motionKeys;
	} else if (actionDesc.motionLabels.size() != actionDesc.motionKeys.size()) {
		actionDesc.motionLabels = actionDesc.motionKeys;
	}

	std::transform(actionDesc.fullscreen_action.begin(), actionDesc.fullscreen_action.end(), actionDesc.fullscreen_action.begin(), tolower);
	int key_idx = 0;

	for (auto &w : Desktop::viewState()->windows()) {
		for (auto &m : State::monitorState()->monitors()) {
			if (w->m_workspace == m->m_activeWorkspace || m->m_activeSpecialWorkspace == w->m_workspace) {
				if (w->isHidden() || !w->m_isMapped)
					continue;
				if (m->m_activeSpecialWorkspace && w->m_workspace != m->m_activeSpecialWorkspace && actionDesc.only_special)
					continue;
				
				std::string kstr = actionDesc.motionKeys.substr(key_idx, 1);
				std::string lstr = actionDesc.motionLabels.substr(key_idx, 1);
				++key_idx;
				addLabelToWindow(w, &actionDesc, kstr, lstr);
			}
		}
	}

	if (!g_pGlobalState->motionLabels.empty())
		HyprlandAPI::invokeHyprctlCommand("dispatch", "submap __easymotionsubmap__");

	return {};
}

bool oneasymotionKeypress(const IKeyboard::SKeyEvent& ev) {
	if (g_pGlobalState->motionLabels.empty()) return false;
	if (g_pInputManager->m_keyboards.empty()) return false;

	SP<IKeyboard> keyboard = g_pInputManager->m_keyboards.front();

	const auto KEYCODE = ev.keycode + 8;

	const xkb_keysym_t KEYSYM = xkb_state_key_get_one_sym(keyboard->m_xkbState, KEYCODE);

	if (ev.state != WL_KEYBOARD_KEY_STATE_PRESSED) return false;

	xkb_keysym_t actionKeysym = 0;
	for (auto &ml : g_pGlobalState->motionLabels) {
		if (ml->m_szKey != "") {
			actionKeysym = xkb_keysym_from_name(ml->m_szKey.c_str(), XKB_KEYSYM_NO_FLAGS);
			if (actionKeysym && (actionKeysym == KEYSYM)) {
				easymotionActionDispatch(ml->m_szKey);
				return true;
			}
		}
	}
	return false;
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

	g_textSize = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:textsize", "Text size", 15,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_textSize);

	g_textColor = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:textcolor", "Text color",
	    Config::ParserUtils::parseColor("rgba(ffffffff)").value_or(0xffffffff),
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_textColor);

	g_bgColor = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:bgcolor", "Background color",
	    Config::ParserUtils::parseColor("rgba(000000ff)").value_or(0xff),
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_bgColor);

	g_textFont = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:textfont", "Text font", "Sans",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_textFont);

	g_textPadding = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:textpadding", "Text padding", "0",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_textPadding);

	g_borderSize = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:bordersize", "Border size", 0,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_borderSize);

	g_borderColor = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:bordercolor", "Border color", "rgba(ffffffff)",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_borderColor);

	g_rounding = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:rounding", "Rounding", 0,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_rounding);

	g_blur = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:blur", "Blur", 0,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_blur);

	g_blurA = makeShared<Config::Values::CFloatValue>(
	    "plugin:easymotion:blurA", "Blur amount", 1.0f,
	    Config::Values::SFloatValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_blurA);

	g_xray = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:xray", "Xray", 0,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_xray);

	g_motionKeys = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:motionkeys", "Motion keys",
	    "abcdefghijklmnopqrstuvwxyz1234567890",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_motionKeys);

	g_motionLabels = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:motionlabels", "Motion labels", "",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_motionLabels);

	g_fullscreenAction = makeShared<Config::Values::CStringValue>(
	    "plugin:easymotion:fullscreen_action", "Fullscreen action", "none",
	    Config::Values::SStringValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_fullscreenAction);

	g_onlySpecial = makeShared<Config::Values::CIntValue>(
	    "plugin:easymotion:only_special", "Only special workspace", 1,
	    Config::Values::SIntValueOptions{});
	HyprlandAPI::addConfigValueV2(PHANDLE, g_onlySpecial);


	g_pGlobalState = makeUnique<SGlobalState>();
	HyprlandAPI::addDispatcherV2(PHANDLE, "easymotion", easymotionDispatch);
	HyprlandAPI::addDispatcherV2(PHANDLE, "easymotionaction", easymotionActionDispatch);
	HyprlandAPI::addDispatcherV2(PHANDLE, "easymotionexit", easymotionExitDispatch);
	static auto KPHOOK = Event::bus()->m_events.input.keyboard.key.listen([&](IKeyboard::SKeyEvent ev, Event::SCallbackInfo& info) {
		info.cancelled = oneasymotionKeypress(ev);
	});
	static auto CRHOOK = Event::bus()->m_events.config.reloaded.listen([&]() { addEasyMotionKeybinds(); });
	HyprlandAPI::reloadConfig();


	return {"hypreasymotion", "Easymotion navigation", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
}