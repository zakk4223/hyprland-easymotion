#include <hyprland/src/plugins/PluginAPI.hpp>
#include <unistd.h>

#include <any>
#include <ranges>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>

#include "easymotionDeco.hpp"
#include "globals.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


void easymotionExitDispatch(std::string args)
{
		for (auto &ml : g_pGlobalState->motionLabels | std::ranges::views::reverse) {
			ml->getOwner()->removeWindowDeco(ml);
		}
		HyprlandAPI::invokeHyprctlCommand("dispatch", "submap reset");
		g_pEventManager->postEvent(SHyprIPCEvent{"easymotionexit", ""});

}

void easymotionActionDispatch(std::string args)
{
	for (auto &ml : g_pGlobalState->motionLabels) {
		if (ml->m_szLabel == args) {
			g_pEventManager->postEvent(SHyprIPCEvent{"easymotionselect", std::format("{},{}", ml->m_szWindowAddress, ml->m_szLabel)});
			g_pKeybindManager->m_mDispatchers["exec"](ml->m_szActionCmd);
			easymotionExitDispatch("");
			break;
		}
	}
}

void addEasyMotionKeybinds()
{

		g_pKeybindManager->addKeybind(SKeybind{"escape", {}, 0, 0, 0, {}, "easymotionexit", "", 0, "__easymotionsubmap__", "", 0, 0, 0, 0, 0, 0, 0, 0});

	  //catchall
		g_pKeybindManager->addKeybind(SKeybind{"", {}, 0, 1, 0, {}, "", "", 0, "__easymotionsubmap__", "", 0, 0, 0, 0, 0, 0, 0, 0});

}


void addLabelToWindow(PHLWINDOW window, SMotionActionDesc *actionDesc, std::string &label)
{
	std::unique_ptr<CHyprEasyLabel> motionlabel = std::make_unique<CHyprEasyLabel>(window, actionDesc);
	motionlabel.get()->m_szLabel = label;
	g_pGlobalState->motionLabels.push_back(motionlabel.get());
	HyprlandAPI::addWindowDecoration(PHANDLE, window, std::move(motionlabel));
}

static bool parseBorderGradient(std::string VALUE, CGradientValueData *DATA) {
    std::string V = VALUE;

    CVarList   varlist(V, 0, ' ');
    DATA->m_vColors.clear();

    std::string parseError = "";

    for (auto& var : varlist) {
        if (var.find("deg") != std::string::npos) {
            // last arg
            try {
                DATA->m_fAngle = std::stoi(var.substr(0, var.find("deg"))) * (PI / 180.0); // radians
            } catch (...) {
                Debug::log(WARN, "Error parsing gradient {}", V);
								return false;
            }

            break;
        }

        if (DATA->m_vColors.size() >= 10) {
            Debug::log(WARN, "Error parsing gradient {}: max colors is 10.", V);
						return false;
            break;
        }

        try {
            DATA->m_vColors.push_back(CColor(configStringToInt(var)));
        } catch (std::exception& e) {
            Debug::log(WARN, "Error parsing gradient {}", V);
        }
    }

    if (DATA->m_vColors.size() == 0) {
        Debug::log(WARN, "Error parsing gradient {}", V);
        DATA->m_vColors.push_back(0); // transparent
    }

    return true;
}

void easymotionDispatch(std::string args)
{
		static auto *const TEXTSIZE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textsize")->getDataStaticPtr();

	  static auto *const TEXTCOLOR = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textcolor")->getDataStaticPtr();
		static auto *const BGCOLOR = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bgcolor")->getDataStaticPtr();
		static auto *const TEXTFONT = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textfont")->getDataStaticPtr();
		static auto *const TEXTPADDING = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textpadding")->getDataStaticPtr();
		static auto *const BORDERSIZE = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bordersize")->getDataStaticPtr();
		static auto *const BORDERCOLOR = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bordercolor")->getDataStaticPtr();
		static auto *const ROUNDING = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:rounding")->getDataStaticPtr();
		static auto *const MOTIONKEYS = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:motionkeys")->getDataStaticPtr();

	CVarList emargs(args, 0, ',');
	SMotionActionDesc actionDesc;

	actionDesc.textSize = **TEXTSIZE;
	actionDesc.textColor = **TEXTCOLOR;
	actionDesc.backgroundColor = **BGCOLOR;
	actionDesc.textFont = *TEXTFONT;
	CVarList cpadding = CVarList(*TEXTPADDING);
	actionDesc.boxPadding.parseGapData(cpadding);
	actionDesc.rounding = **ROUNDING;
	actionDesc.borderSize = **BORDERSIZE;
	if(!parseBorderGradient(*BORDERCOLOR, &actionDesc.borderColor)) {
		actionDesc.borderColor.m_vColors.clear();
		actionDesc.borderColor.m_fAngle = 0;
	}
	actionDesc.motionKeys = *MOTIONKEYS;


	for(size_t i = 0; i < emargs.size(); i++)
	{

		CVarList kv(emargs[i], 2, ':');
		if (kv[0] == "action") {
			actionDesc.commandString = kv[1];
		} else if (kv[0] == "textsize") {
			actionDesc.textSize = configStringToInt(kv[1]);
		} else if (kv[0] == "textcolor") {
			actionDesc.textColor = CColor(configStringToInt(kv[1]));
		} else if (kv[0] == "bgcolor") {
			actionDesc.backgroundColor = CColor(configStringToInt(kv[1]));
		} else if (kv[0] == "textfont") {
			actionDesc.textFont = kv[1];
		} else if (kv[0] == "textpadding") {
			CVarList padVars = CVarList(kv[1], 0, 's');
			actionDesc.boxPadding.parseGapData(padVars);
		} else if (kv[0] == "rounding") {
			actionDesc.rounding = configStringToInt(kv[1]);
		} else if (kv[0] == "bordersize") {
			actionDesc.borderSize = configStringToInt(kv[1]);
		} else if (kv[0] == "bordercolor") {
			CVarList varlist(kv[1], 0, 's');
			actionDesc.borderColor.m_vColors.clear();
			actionDesc.borderColor.m_fAngle = 0;
			if(!parseBorderGradient(kv[1], &actionDesc.borderColor)) {
				actionDesc.borderColor.m_vColors.clear();
				actionDesc.borderColor.m_fAngle = 0;
			}
		} else if (kv[0] == "motionkeys") {
				actionDesc.motionKeys = kv[1];
		}
	}

	int key_idx = 0;

	for (auto &w : g_pCompositor->m_vWindows) {
		for (auto &m : g_pCompositor->m_vMonitors) {
			if (w->m_pWorkspace == m->activeWorkspace || m->activeSpecialWorkspace == w->m_pWorkspace) {
					if (w->isHidden() || !w->m_bIsMapped || w->m_bFadingOut)
						continue;
                    if (w->m_pWorkspace->m_bHasFullscreenWindow && 
                        g_pCompositor->getFullscreenWindowOnWorkspace(w->workspaceID()) != w) {
                        continue;
                    }
					std::string lstr = actionDesc.motionKeys.substr(key_idx++, 1);
					addLabelToWindow(w, &actionDesc, lstr);
			}
		}
	}

	if (!g_pGlobalState->motionLabels.empty())
		HyprlandAPI::invokeHyprctlCommand("dispatch", "submap __easymotionsubmap__");
}
	
bool oneasymotionKeypress(void *self, std::any data) {

	if (g_pGlobalState->motionLabels.empty()) return false;

	auto map = std::any_cast<std::unordered_map<std::string, std::any>>(data);
	std::any kany = map["keyboard"];
	IKeyboard::SKeyEvent ev = std::any_cast<IKeyboard::SKeyEvent>(map["event"]);
	SP<IKeyboard>keyboard = std::any_cast<SP<IKeyboard>>(kany);



	const auto KEYCODE = ev.keycode + 8;

	const xkb_keysym_t KEYSYM = xkb_state_key_get_one_sym(keyboard->xkbState, KEYCODE);

	if (ev.state != WL_KEYBOARD_KEY_STATE_PRESSED) return false;

	xkb_keysym_t actionKeysym = 0;
	for (auto &ml : g_pGlobalState->motionLabels) {
		if (ml->m_szLabel != "") {
			actionKeysym = xkb_keysym_from_name(ml->m_szLabel.c_str(), XKB_KEYSYM_NO_FLAGS);
			if (actionKeysym && (actionKeysym == KEYSYM)) {
				easymotionActionDispatch(ml->m_szLabel);
				return true;
			}
		}
	}
	return false;
}


APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
    PHANDLE = handle;

		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textsize", Hyprlang::INT{15});

		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textcolor", Hyprlang::INT{configStringToInt("rgba(ffffffff)")});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bgcolor", Hyprlang::INT{configStringToInt("rgba(000000ff)")});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textfont", Hyprlang::STRING{"Sans"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textpadding", Hyprlang::STRING{"0"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bordersize", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bordercolor", Hyprlang::STRING{"rgba(ffffffff)"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:rounding", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:motionkeys", Hyprlang::STRING{"abcdefghijklmnopqrstuvwxyz1234567890"});


		g_pGlobalState = std::make_unique<SGlobalState>();
		HyprlandAPI::addDispatcher(PHANDLE, "easymotion", easymotionDispatch);
		HyprlandAPI::addDispatcher(PHANDLE, "easymotionaction", easymotionActionDispatch);
		HyprlandAPI::addDispatcher(PHANDLE, "easymotionexit", easymotionExitDispatch);
		static auto KPHOOK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "keyPress", [&](void *self, SCallbackInfo &info, std::any data) {
			info.cancelled = oneasymotionKeypress(self, data);
	});
	  static auto CRHOOK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", [&](void *self, SCallbackInfo&, std::any data) {addEasyMotionKeybinds();});
    HyprlandAPI::reloadConfig();


    return {"hypreasymotion", "Easymotion navigation", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
}
