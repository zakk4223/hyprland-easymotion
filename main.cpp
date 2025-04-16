#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <string>
#include <unistd.h>

#include <any>
#include <ranges>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/Window.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/managers/EventManager.hpp>
#include <strings.h>

#include "easymotionDeco.hpp"
#include "globals.hpp"

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() {
    return HYPRLAND_API_VERSION;
}


SDispatchResult easymotionExitDispatch(std::string args)
{
		for (auto &ml : g_pGlobalState->motionLabels | std::ranges::views::reverse) {
        if (ml->m_origFSMode != ml->getOwner()->m_sFullscreenState.internal)
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
		if (ml->m_szLabel == args) {
			g_pEventManager->postEvent(SHyprIPCEvent{"easymotionselect", std::format("{},{}", ml->m_szWindowAddress, ml->m_szLabel)});
			g_pKeybindManager->m_mDispatchers["exec"](ml->m_szActionCmd);
			easymotionExitDispatch("");
			break;
		}
	}

  return {};
}

void addEasyMotionKeybinds()
{

		g_pKeybindManager->addKeybind(SKeybind{"escape", {}, 0, 0, 0, {}, "easymotionexit", "", 0, "__easymotionsubmap__", "", 0, 0, 0, 0, 0, 0, 0, 0});
	  //catchall
		g_pKeybindManager->addKeybind(SKeybind{"", {}, 0, 1, 0, {}, "", "", 0, "__easymotionsubmap__", "", 0, 0, 0, 0, 0, 0, 0, 0});

}


void addLabelToWindow(PHLWINDOW window, SMotionActionDesc *actionDesc, std::string &label)
{
	UP<CHyprEasyLabel> motionlabel = makeUnique<CHyprEasyLabel>(window, actionDesc);
	motionlabel->m_szLabel = label;
	g_pGlobalState->motionLabels.emplace_back(motionlabel);
  motionlabel->m_self = motionlabel;
  motionlabel->draw(window->m_pMonitor.lock(), 1.0);
  motionlabel->m_origFSMode = window->m_sFullscreenState.internal;
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
            DATA->m_vColors.push_back(CHyprColor(configStringToInt(var).value_or(0)));
        } catch (std::exception& e) {
            Debug::log(WARN, "Error parsing gradient {}", V);
        }
    }

    if (DATA->m_vColors.size() == 0) {
        Debug::log(WARN, "Error parsing gradient {}", V);
        DATA->m_vColors.push_back(0); // transparent
    }

    DATA->updateColorsOk();
    return true;
}

SDispatchResult easymotionDispatch(std::string args)
{
		static auto *const TEXTSIZE = (Hyprlang::INT* const*)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textsize")->getDataStaticPtr();

	  static auto *const TEXTCOLOR = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textcolor")->getDataStaticPtr();
		static auto *const BGCOLOR = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bgcolor")->getDataStaticPtr();
		static auto *const TEXTFONT = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textfont")->getDataStaticPtr();
		static auto *const TEXTPADDING = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:textpadding")->getDataStaticPtr();
		static auto *const BORDERSIZE = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bordersize")->getDataStaticPtr();
		static auto *const BORDERCOLOR = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:bordercolor")->getDataStaticPtr();
		static auto *const ROUNDING = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:rounding")->getDataStaticPtr();
		static auto *const BLUR = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:blur")->getDataStaticPtr();
		static auto *const XRAY = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:xray")->getDataStaticPtr();
		static auto *const BLURA = (Hyprlang::FLOAT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:blurA")->getDataStaticPtr();
		static auto *const MOTIONKEYS = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:motionkeys")->getDataStaticPtr();
		static auto *const FSACTION = (Hyprlang::STRING const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:fullscreen_action")->getDataStaticPtr();
		static auto *const ONLYSPECIAL = (Hyprlang::INT* const *)HyprlandAPI::getConfigValue(PHANDLE, "plugin:easymotion:only_special")->getDataStaticPtr();


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
  actionDesc.blur = **BLUR;
  actionDesc.xray = **XRAY;
  actionDesc.blurA = **BLURA;
  actionDesc.fullscreen_action = std::string(*FSACTION);
  actionDesc.only_special = **ONLYSPECIAL;


	for(size_t i = 0; i < emargs.size(); i++)
	{

		CVarList kv(emargs[i], 2, ':');
		if (kv[0] == "action") {
			actionDesc.commandString = kv[1];
		} else if (kv[0] == "textsize") {
			actionDesc.textSize = configStringToInt(kv[1]).value_or(15);
		} else if (kv[0] == "textcolor") {
			actionDesc.textColor = CHyprColor(configStringToInt(kv[1]).value_or(0xffffffff));
		} else if (kv[0] == "bgcolor") {
			actionDesc.backgroundColor = CHyprColor(configStringToInt(kv[1]).value_or(0));
		} else if (kv[0] == "textfont") {
			actionDesc.textFont = kv[1];
		} else if (kv[0] == "textpadding") {
			CVarList padVars = CVarList(kv[1], 0, 's');
			actionDesc.boxPadding.parseGapData(padVars);
		} else if (kv[0] == "rounding") {
			actionDesc.rounding = configStringToInt(kv[1]).value_or(0);
		} else if (kv[0] == "bordersize") {
			actionDesc.borderSize = configStringToInt(kv[1]).value_or(0);
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
    } else if (kv[0] == "blur") {
      actionDesc.blur = configStringToInt(kv[1]).value_or(1);
    } else if (kv[0] == "xray") {
      actionDesc.xray = configStringToInt(kv[1]).value_or(1);
    } else if (kv[0] == "blurA") {
      try {
        actionDesc.blurA = std::stof(kv[1]);
      } catch (const std::invalid_argument& ia) {
        actionDesc.blurA = 1.0f;
      }
		} else if (kv[0] == "fullscreen_action") {
      actionDesc.fullscreen_action = kv[1];
    } else if (kv[0] == "only_special") {
      actionDesc.only_special = configStringToInt(kv[1]).value_or(1);
    }
	}

  std::transform(actionDesc.fullscreen_action.begin(), actionDesc.fullscreen_action.end(), actionDesc.fullscreen_action.begin(), tolower);
	int key_idx = 0;

	for (auto &w : g_pCompositor->m_vWindows) {
		for (auto &m : g_pCompositor->m_vMonitors) {
			if (w->m_pWorkspace == m->activeWorkspace || m->activeSpecialWorkspace == w->m_pWorkspace) {
					if (w->isHidden() || !w->m_bIsMapped || w->m_bFadingOut)
						continue;
          if (m->activeSpecialWorkspace && w->m_pWorkspace != m->activeSpecialWorkspace && actionDesc.only_special)
            continue;
					std::string lstr = actionDesc.motionKeys.substr(key_idx++, 1);
					addLabelToWindow(w, &actionDesc, lstr);
			}
		}
	}

	if (!g_pGlobalState->motionLabels.empty())
		HyprlandAPI::invokeHyprctlCommand("dispatch", "submap __easymotionsubmap__");

  return {};
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

		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textcolor", Hyprlang::INT{configStringToInt("rgba(ffffffff)").value_or(0xffffffff)});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bgcolor", Hyprlang::INT{configStringToInt("rgba(000000ff)").value_or(0xff)});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textfont", Hyprlang::STRING{"Sans"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:textpadding", Hyprlang::STRING{"0"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bordersize", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:bordercolor", Hyprlang::STRING{"rgba(ffffffff)"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:rounding", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:blur", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:blurA", Hyprlang::FLOAT{1.0f});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:xray", Hyprlang::INT{0});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:motionkeys", Hyprlang::STRING{"abcdefghijklmnopqrstuvwxyz1234567890"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:fullscreen_action", Hyprlang::STRING{"none"});
		HyprlandAPI::addConfigValue(PHANDLE, "plugin:easymotion:only_special", Hyprlang::INT{1});


		g_pGlobalState = makeUnique<SGlobalState>();
		HyprlandAPI::addDispatcherV2(PHANDLE, "easymotion", easymotionDispatch);
		HyprlandAPI::addDispatcherV2(PHANDLE, "easymotionaction", easymotionActionDispatch);
		HyprlandAPI::addDispatcherV2(PHANDLE, "easymotionexit", easymotionExitDispatch);
		static auto KPHOOK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "keyPress", [&](void *self, SCallbackInfo &info, std::any data) {
			info.cancelled = oneasymotionKeypress(self, data);
	});
	  static auto CRHOOK = HyprlandAPI::registerCallbackDynamic(PHANDLE, "configReloaded", [&](void *self, SCallbackInfo&, std::any data) {addEasyMotionKeybinds();});
    HyprlandAPI::reloadConfig();


    return {"hypreasymotion", "Easymotion navigation", "Zakk", "1.0"};
}

APICALL EXPORT void PLUGIN_EXIT() {
}
