#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <string>
#include <unistd.h>

#include <any>
#include <ranges>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/config/values/types/BoolValue.hpp>
#include <hyprland/src/config/values/types/ColorValue.hpp>
#include <hyprland/src/config/values/types/CssGapValue.hpp>
#include <hyprland/src/config/values/types/GradientValue.hpp>
#include <hyprland/src/config/values/types/FloatValue.hpp>
#include <hyprland/src/config/values/types/IntValue.hpp>
#include <hyprland/src/config/values/types/StringValue.hpp>
#include <hyprland/src/config/shared/parserUtils/ParserUtils.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <hyprland/src/managers/input/InputManager.hpp>
#include <hyprland/src/event/EventBus.hpp>
#include <hyprland/src/debug/log/Logger.hpp>
#include <hyprutils/string/VarList.hpp>
#include <strings.h>

#include "easymotionDeco.hpp"
#include "globals.hpp"

using namespace Hyprutils::String;
using namespace Render::GL;

static struct {
	SP<Config::Values::CStringValue> motionKeys, motionLabels, textFont;
	SP<Config::Values::CIntValue> textSize;
	SP<Config::Values::CColorValue> textColor, bgColor;
	SP<Config::Values::CCssGapValue> textPadding;
	SP<Config::Values::CIntValue> borderSize;
	SP<Config::Values::CGradientValue> borderColor;
	SP<Config::Values::CIntValue> rounding;
	SP<Config::Values::CBoolValue> blur;
	SP<Config::Values::CFloatValue> blurA;
	SP<Config::Values::CBoolValue> xray;
	SP<Config::Values::CStringValue> fullscreenAction;
	SP<Config::Values::CBoolValue> onlySpecial;
} configValues;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
	return HYPRLAND_API_VERSION;
}


SDispatchResult easymotionExitDispatch(std::string args)
{
	for (auto &ml : g_pGlobalState->motionLabels | std::ranges::views::reverse) {
		if (ml->m_origFSMode != ml->getOwner()->m_fullscreenState.internal)
			g_pCompositor->setWindowFullscreenInternal(ml->getOwner(), ml->m_origFSMode);
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
	//catchall
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
	motionlabel->m_origFSMode = window->m_fullscreenState.internal;
	if ((motionlabel->m_origFSMode != eFullscreenMode::FSMODE_NONE) && (actionDesc->fullscreen_action != "none"))
	{
		if (actionDesc->fullscreen_action == "maximize")
		{
			g_pCompositor->setWindowFullscreenInternal(window, FSMODE_MAXIMIZED);
		} else if (actionDesc->fullscreen_action == "toggle") {
			g_pCompositor->setWindowFullscreenInternal(window, FSMODE_NONE);
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
			// last arg
			try {
				DATA->m_angle = std::stoi(var.substr(0, var.find("deg"))) * (PI / 180.0); // radians
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
		DATA->m_colors.push_back(0); // transparent
	}

	DATA->updateColorsOk();
	return true;
}

SDispatchResult easymotionDispatch(std::string args)
{
	CVarList emargs(args, 0, ',');
	SMotionActionDesc actionDesc;

	actionDesc.textSize = configValues.textSize->value();
	actionDesc.textColor = configValues.textColor->value();
	actionDesc.backgroundColor = configValues.bgColor->value();
	actionDesc.textFont = configValues.textFont->value();
	actionDesc.boxPadding = configValues.textPadding->value();
	actionDesc.rounding = configValues.rounding->value();
	actionDesc.borderSize = configValues.borderSize->value();
	actionDesc.borderColor = configValues.borderColor->value();
	actionDesc.motionKeys = configValues.motionKeys->value();
	actionDesc.motionLabels = configValues.motionLabels->value();
	actionDesc.blur = configValues.blur->value();
	actionDesc.xray = configValues.xray->value();
	actionDesc.blurA = configValues.blurA->value();
	actionDesc.fullscreen_action = configValues.fullscreenAction->value();
	actionDesc.only_special = configValues.onlySpecial->value();


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
		//TODO: add a warning here
		actionDesc.motionLabels = actionDesc.motionKeys;
	}

	std::transform(actionDesc.fullscreen_action.begin(), actionDesc.fullscreen_action.end(), actionDesc.fullscreen_action.begin(), tolower);
	int key_idx = 0;

	for (auto &w : g_pCompositor->m_windows) {
		for (auto &m : g_pCompositor->m_monitors) {
			if (w->m_workspace == m->m_activeWorkspace || m->m_activeSpecialWorkspace == w->m_workspace) {
				if (w->isHidden() || !w->m_isMapped || w->m_fadingOut)
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

	configValues.motionKeys = makeShared<Config::Values::CStringValue>("plugin:easymotion:motionkeys", "which keys to use for selecting windows", "abcdefghijklmnopqrstuvwxyz1234567890");
	configValues.motionLabels = makeShared<Config::Values::CStringValue>("plugin:easymotion:motionlabels", "which keys to use for labeling windows", "");
	configValues.textFont = makeShared<Config::Values::CStringValue>("plugin:easymotion:textfont", "font face for window label text", "Sans");
	configValues.textSize = makeShared<Config::Values::CIntValue>("plugin:easymotion:textsize", "font size of window label text", 15, Config::Values::SIntValueOptions{.min = 1} );
	configValues.textColor = makeShared<Config::Values::CColorValue>("plugin:easymotion:textcolor", "color of window label text", 0xffffffff);
	configValues.bgColor = makeShared<Config::Values::CColorValue>("plugin:easymotion:bgcolor", "background color of window label", 0x000000ff);
	configValues.textPadding = makeShared<Config::Values::CCssGapValue>("plugin:easymotion:textpadding", "padding around window label text", 0);
	configValues.borderSize = makeShared<Config::Values::CIntValue>("plugin:easymotion:bordersize", "size of window label border", 0);
	configValues.borderColor = makeShared<Config::Values::CGradientValue>("plugin:easymotion:bordercolor", "color of window label border", 0xffffffff);
	configValues.rounding = makeShared<Config::Values::CIntValue>("plugin:easymotion:rounding", "rounding radius of window label corners", 0);
	configValues.blur = makeShared<Config::Values::CBoolValue>("plugin:easymotion:blur", "whether to blur under window label", false);
	configValues.blurA = makeShared<Config::Values::CFloatValue>("plugin:easymotion:blurA", "window label blur alpha", 1.F, Config::Values::SFloatValueOptions{.min = 0.F, .max = 1.F} );
	configValues.xray = makeShared<Config::Values::CBoolValue>("plugin:easymotion:xrap", "window label blur xray", false);
	configValues.fullscreenAction = makeShared<Config::Values::CStringValue>("plugin:easymotion:fullscreen_action", "what to do if a window is fullscreen", "none");
	configValues.onlySpecial = makeShared<Config::Values::CBoolValue>("plugin:easymotion:only_special", "if monitor has a special workspace focused, label its windows only", true);

	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.onlySpecial);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.motionKeys);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.motionLabels);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.textFont);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.textSize);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.textColor);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.bgColor);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.textPadding);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.borderSize);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.borderColor);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.rounding);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.blur);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.blurA);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.xray);
	HyprlandAPI::addConfigValueV2(PHANDLE, configValues.fullscreenAction);

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
