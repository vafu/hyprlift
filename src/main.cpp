#define WLR_USE_UNSTABLE

#include <unistd.h>
#include <vector>

#include <any>
#include <hyprland/src/includes.hpp>
#include <sstream>

#define private public
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/managers/AnimationManager.hpp>
#include <hyprland/src/managers/LayoutManager.hpp>
#include <hyprland/src/managers/eventLoop/EventLoopManager.hpp>
#undef private

#include "globals.hpp"

#include <hyprutils/animation/BezierCurve.hpp>
#include <hyprutils/string/VarList.hpp>
using namespace Hyprutils::String;
using namespace Hyprutils::Animation;

// Do NOT change this function.
APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

void resetWindowScale(PHLWINDOW pWin) {
  if (!pWin || !pWin->m_isMapped || pWin->m_fadingOut)
    return;

  // Use the "in" animation config for a smooth return to normal size
  const auto IN_ANIM =
      g_pConfigManager->getAnimationPropertyConfig("hyprliftIn");
  pWin->m_realPosition->setConfig(IN_ANIM);
  pWin->m_realSize->setConfig(IN_ANIM);

  // The ->goal() is the position/size the layout manager wants the window to
  // have. We set the animated variables to this goal to smoothly animate back
  // to the original state.
  *pWin->m_realPosition = pWin->m_realPosition->goal();
  *pWin->m_realSize = pWin->m_realSize->goal();
}

void scaleWindow(PHLWINDOW pWin, float scale) {
  if (!pWin || !pWin->m_isMapped || pWin->m_fadingOut)
    return;

  // Use the "out" animation config for a smooth scaling down
  const auto OUT_ANIM =
      g_pConfigManager->getAnimationPropertyConfig("hyprliftOut");
  pWin->m_realPosition->setConfig(OUT_ANIM);
  pWin->m_realSize->setConfig(OUT_ANIM);

  // This is the box the layout manager wants the window to have (full size)
  const auto ORIGINAL_BOX =
      CBox{pWin->m_realPosition->goal(), pWin->m_realSize->goal()};

  // Create a new box, scaled from the center
  auto scaledBox = ORIGINAL_BOX.copy().scaleFromCenter(scale);

  // Set the animated variables to the new scaled-down box
  *pWin->m_realPosition = scaledBox.pos();
  *pWin->m_realSize = scaledBox.size();
}

static void onFocusChange(PHLWINDOW window) {
  if (!window)
    return;

  static const auto *PUNFOCUSED_SCALE =
      (Hyprlang::FLOAT *const *)HyprlandAPI::getConfigValue(
          PHANDLE, "plugin:hyprlift:unfocused_scale")
          ->getDataStaticPtr();
  const float scale = **PUNFOCUSED_SCALE;
  const int activeWorkspaceID = window->workspaceID();
  for (const auto &pWin : g_pCompositor->m_windows) {
    if (pWin->workspaceID() != activeWorkspaceID)
      continue;
    if (pWin == window) {
      // This is the newly focused window, so reset its scale to 1.0
      resetWindowScale(pWin);
    } else {
      // This is an unfocused window, so scale it down
      scaleWindow(pWin, scale);
    }
  }
}

static void onWorkspaceChange(CWorkspace *activeWorkspace) {
  if (!activeWorkspace)
    return;

  // Loop through ALL windows managed by the compositor
  for (const auto &pWin : g_pCompositor->m_windows) {
    // If the window is valid and NOT on the currently active workspace, reset
    // its scale.
    if (validMapped(pWin) && pWin->workspaceID() != activeWorkspace->m_id) {
      resetWindowScale(pWin);
    }
  }
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
  PHANDLE = handle;

  const std::string HASH = __hyprland_api_get_hash();

  if (HASH != GIT_COMMIT_HASH) {
    HyprlandAPI::addNotification(
        PHANDLE,
        "[hyprlift] Failure in initialization: Version mismatch (headers "
        "ver is not equal to running hyprland ver)",
        CHyprColor{1.0, 0.2, 0.2, 1.0}, 5000);
    throw std::runtime_error("[hyprlift] Version mismatch");
  }

  static auto P = HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "activeWindow",
      [&](void *self, SCallbackInfo &info, std::any data) {
        onFocusChange(std::any_cast<PHLWINDOW>(data));
      });

  static auto PP = HyprlandAPI::registerCallbackDynamic(
      PHANDLE, "workspace",
      [&](void *self, SCallbackInfo &info, std::any data) {
        onWorkspaceChange(std::any_cast<CWorkspace *>(data));
      });

  HyprlandAPI::addConfigValue(PHANDLE, "plugin:hyprlift:unfocused_scale",
                              Hyprlang::FLOAT{0.95F});

  // this will not be cleaned up after we are unloaded but it doesn't really
  // matter, as if we create this again it will just overwrite the old one.
  g_pConfigManager->m_animationTree.createNode("hyprliftIn", "windowsIn");
  g_pConfigManager->m_animationTree.createNode("hyprliftOut", "windowsOut");

  return {"hyprlift", "Lifts focused window over others", "vafu", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
  // reset the callbacks to avoid crashes
  for (const auto &w : g_pCompositor->m_windows) {
    if (!validMapped(w))
      continue;

    w->m_realSize->setCallbackOnEnd(nullptr);
    w->m_realPosition->setCallbackOnEnd(nullptr);
    w->m_activeInactiveAlpha->setCallbackOnEnd(nullptr);
  }
}
