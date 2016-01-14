/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include "CompizShortcutModeller.h"
#include "ShortcutHint.h"
#include "ShortcutHintPrivate.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/XKeyboardUtil.h"

namespace unity
{
namespace shortcut
{
namespace
{
  // Compiz' plug-in names
  const std::string CORE_PLUGIN_NAME = "core";
  const std::string EXPO_PLUGIN_NAME = "expo";
  const std::string GRID_PLUGIN_NAME = "grid";
  const std::string MOVE_PLUGIN_NAME = "move";
  const std::string RESIZE_PLUGIN_NAME = "resize";
  const std::string SCALE_PLUGIN_NAME = "scale";
  const std::string UNITYSHELL_PLUGIN_NAME = "unityshell";
  const std::string WALL_PLUGIN_NAME = "wall";

  // Compiz Core Options
  const std::string CORE_OPTION_SHOW_DESKTOP_KEY = "show_desktop_key";
  const std::string CORE_OPTION_MAXIMIZE_KEY = "maximize_window_key";
  const std::string CORE_OPTION_RESTORE_MINIMIZE_KEY = "unmaximize_or_minimize_window_key";
  const std::string CORE_OPTION_CLOSE_WINDOW_KEY = "close_window_key";
  const std::string CORE_OPTION_WINDOW_MENU_KEY = "window_menu_key";

  // Compiz Expo Options
  const std::string EXPO_OPTION_EXPO_KEY = "expo_key";

  // Compiz Grid Options
  const std::string GRID_OPTION_LEFT_MAXIMIZE = "left_maximize";

  // Compiz Move Options
  const std::string MOVE_OPTION_INITIATE_BUTTON = "initiate_button";

  // Compiz Resize Options
  const std::string RESIZE_OPTION_INITIATE_BUTTON = "initiate_button";

  // Compiz Scale Options
  const std::string SCALE_OPTION_INITIATE_KEY = "initiate_key";
  const std::string SCALE_OPTION_INITIATE_ALL_KEY = "initiate_all_key";

  // Compiz Unityshell Options
  const std::string UNITYSHELL_OPTION_SHOW_LAUNCHER = "show_launcher";
  const std::string UNITYSHELL_OPTION_KEYBOARD_FOCUS = "keyboard_focus";
  const std::string UNITYSHELL_OPTION_LAUNCHER_SWITCHER_FORWARD = "launcher_switcher_forward";
  const std::string UNITYSHELL_OPTION_SHOW_HUD = "show_hud";
  const std::string UNITYSHELL_OPTION_PANEL_FIRST_MENU = "panel_first_menu";
  const std::string UNITYSHELL_OPTION_SHOW_MENUS = "show_menu_bar";
  const std::string UNITYSHELL_OPTION_SPREAD_APP_WINDOWS = "spread_app_windows";
  const std::string UNITYSHELL_OPTION_SPREAD_APP_WINDOWS_ANYWHERE = "spread_app_windows_anywhere";
  const std::string UNITYSHELL_OPTION_ALT_TAB_FORWARD = "alt_tab_forward";
  const std::string UNITYSHELL_OPTION_ALT_TAB_FORWARD_ALL = "alt_tab_forward_all";
  const std::string UNITYSHELL_OPTION_ALT_TAB_NEXT_WINDOW = "alt_tab_next_window";

  // Compiz Wall Options
  const std::string WALL_OPTION_LEFT_KEY = "left_key";
  const std::string WALL_OPTION_LEFT_WINDOW_KEY = "left_window_key";
}

CompizModeller::CompizModeller()
{
  auto& wm = WindowManager::Default();
  BuildModel(wm.GetViewportHSize(), wm.GetViewportVSize());
  wm.viewport_layout_changed.connect(sigc::mem_fun(this, &CompizModeller::BuildModel));
}

Model::Ptr CompizModeller::GetCurrentModel() const
{
  return model_;
}

void CompizModeller::BuildModel(int hsize, int vsize)
{
  std::list<shortcut::AbstractHint::Ptr> hints;
  bool workspace_enabled = (hsize * vsize > 1);

  if (workspace_enabled)
  {
    AddLauncherHints(hints);
    AddDashHints(hints);
    AddMenuHints(hints);
    AddSwitcherHints(hints, workspace_enabled);
    AddWorkspaceHints(hints);
  }
  else
  {
    AddLauncherHints(hints);
    AddMenuHints(hints);
    AddSwitcherHints(hints, workspace_enabled);
    AddDashHints(hints);
  }

  AddWindowsHints(hints, workspace_enabled);

  model_ = std::make_shared<shortcut::Model>(hints);
  model_changed.emit(model_);
}

void CompizModeller::AddLauncherHints(std::list<shortcut::AbstractHint::Ptr> &hints)
{
  static const std::string launcher(_("Launcher"));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" (Hold)"),
                                                   _("Opens the Launcher, displays shortcuts."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", "",
                                                   _("Opens Launcher keyboard navigation mode."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_KEYBOARD_FOCUS));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", "",
                                                   _("Switches applications via the Launcher."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_LAUNCHER_SWITCHER_FORWARD));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" + 1 to 9"),
                                                   _("Same as clicking on a Launcher icon."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", _(" + Shift + 1 to 9"),
                                                   _("Opens a new window in the app."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(launcher, "", " + T",
                                                   _("Opens the Trash."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));
}

void CompizModeller::AddDashHints(std::list<shortcut::AbstractHint::Ptr> &hints)
{
  static const std::string dash( _("Dash"));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", _(" (Tap)"),
                                                   _("Opens the Dash Home."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", " + A",
                                                   _("Opens the Dash App Lens."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", " + F",
                                                   _("Opens the Dash Files Lens."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", " + M",
                                                   _("Opens the Dash Music Lens."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", " + C",
                                                   _("Opens the Dash Photo Lens."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", " + V",
                                                   _("Opens the Dash Video Lens."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_LAUNCHER));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                   _("Switches between Lenses."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Ctrl + Tab")));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                   _("Moves the focus."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Arrow Keys")));

  hints.push_back(std::make_shared<shortcut::Hint>(dash, "", "",
                                                   _("Opens the currently focused item."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Enter")));
}

void CompizModeller::AddMenuHints(std::list<shortcut::AbstractHint::Ptr> &hints)
{
  static const std::string menubar(_("HUD & Menu Bar"));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", _(" (Tap)"),
                                                   _("Opens the HUD."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_HUD));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", _(" (Hold)"),
                                                   _("Reveals the application menu."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SHOW_MENUS));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                   _("Opens the indicator menu."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_PANEL_FIRST_MENU));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                   _("Moves focus between indicators."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Cursor Left or Right")));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                   _("Take a screenshot."),
                                                   shortcut::OptionType::GNOME,
                                                   "screenshot"));

  hints.push_back(std::make_shared<shortcut::Hint>(menubar, "", "",
                                                   _("Take a screenshot of the current window."),
                                                   shortcut::OptionType::GNOME,
                                                   "window-screenshot"));
}

void CompizModeller::AddSwitcherHints(std::list<shortcut::AbstractHint::Ptr> &hints, bool ws_enabled)
{
  static const std::string switching(_("Switching"));

  auto switcher_init = std::make_shared<shortcut::Hint>(switching, "", "",
                                                        _("Switches between applications."),
                                                        shortcut::OptionType::COMPIZ_KEY,
                                                        UNITYSHELL_PLUGIN_NAME,
                                                        UNITYSHELL_OPTION_ALT_TAB_FORWARD);
  hints.push_back(switcher_init);

  if (ws_enabled)
  {
    hints.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                     _("Switches between applications from all workspaces."),
                                                     shortcut::OptionType::COMPIZ_KEY,
                                                     UNITYSHELL_PLUGIN_NAME,
                                                     UNITYSHELL_OPTION_ALT_TAB_FORWARD_ALL));
  }

  hints.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                   _("Switches windows of current applications."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_ALT_TAB_NEXT_WINDOW));

  hints.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                   _("Moves the focus."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Cursor Left or Right")));

  hints.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                   _("Enter / Exit from spread mode or Select windows."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Cursor Up or Down")));

  if (Display *dpy = nux::GetGraphicsDisplay()->GetX11Display())
  {
    if (const char* key = XKeysymToString(keyboard::get_key_right_to_key_symbol(dpy, XStringToKeysym("Tab"))))
    {
      std::string closekey = key;
      switcher_init->Fill();
      auto const& switcher_init_key = switcher_init->shortkey();
      auto meta_separator = switcher_init_key.find("+");

      if (meta_separator != std::string::npos)
        closekey = switcher_init_key.substr(0, meta_separator-1) + " + " + closekey;

      hints.push_back(std::make_shared<shortcut::Hint>(switching, "", "",
                                                       _("Closes the selected application / window."),
                                                       shortcut::OptionType::HARDCODED,
                                                       impl::ProperCase(closekey)));
    }
  }
}

void CompizModeller::AddWorkspaceHints(std::list<shortcut::AbstractHint::Ptr> &hints)
{
  static const std::string workspaces(_("Workspaces"));

  hints.push_back(std::make_shared<shortcut::Hint>(workspaces, "", "",
                                                   _("Switches between workspaces."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   EXPO_PLUGIN_NAME,
                                                   EXPO_OPTION_EXPO_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(workspaces, "", _(" + Arrow Keys"),
                                                   _("Switches workspaces."),
                                                   shortcut::OptionType::COMPIZ_METAKEY,
                                                   WALL_PLUGIN_NAME,
                                                   WALL_OPTION_LEFT_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(workspaces, "", _(" + Arrow Keys"),
                                                   _("Moves focused window to another workspace."),
                                                   shortcut::OptionType::COMPIZ_METAKEY,
                                                   WALL_PLUGIN_NAME,
                                                   WALL_OPTION_LEFT_WINDOW_KEY));
}

void CompizModeller::AddWindowsHints(std::list<shortcut::AbstractHint::Ptr> &hints, bool ws_enabled)
{
  static const std::string windows(_("Windows"));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   (ws_enabled ?
                                                    _("Spreads all windows in the current workspace.") :
                                                    _("Spreads all windows.")),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   SCALE_PLUGIN_NAME,
                                                   SCALE_OPTION_INITIATE_KEY));

  if (ws_enabled)
  {
    hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                     _("Spreads all windows in all the workspaces."),
                                                     shortcut::OptionType::COMPIZ_KEY,
                                                     SCALE_PLUGIN_NAME,
                                                     SCALE_OPTION_INITIATE_ALL_KEY));
  }

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   (ws_enabled ?
                                                    _("Spreads all windows of the focused application in the current workspace.") :
                                                    _("Spreads all windows of the focused application.")),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   UNITYSHELL_PLUGIN_NAME,
                                                   UNITYSHELL_OPTION_SPREAD_APP_WINDOWS));

  if (ws_enabled)
  {
    hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                     _("Spreads all windows of the focused application in all the workspaces."),
                                                     shortcut::OptionType::COMPIZ_KEY,
                                                     UNITYSHELL_PLUGIN_NAME,
                                                     UNITYSHELL_OPTION_SPREAD_APP_WINDOWS_ANYWHERE));
  }

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Minimises all windows."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   CORE_PLUGIN_NAME,
                                                   CORE_OPTION_SHOW_DESKTOP_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Maximises the current window."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   CORE_PLUGIN_NAME,
                                                   CORE_OPTION_MAXIMIZE_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Restores or minimises the current window."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   CORE_PLUGIN_NAME,
                                                   CORE_OPTION_RESTORE_MINIMIZE_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" or Right"),
                                                   _("Semi-maximise the current window."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   GRID_PLUGIN_NAME,
                                                   GRID_OPTION_LEFT_MAXIMIZE));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Closes the current window."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   CORE_PLUGIN_NAME,
                                                   CORE_OPTION_CLOSE_WINDOW_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Opens the window accessibility menu."),
                                                   shortcut::OptionType::COMPIZ_KEY,
                                                   CORE_PLUGIN_NAME,
                                                   CORE_OPTION_WINDOW_MENU_KEY));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", "",
                                                   _("Places the window in corresponding position."),
                                                   shortcut::OptionType::HARDCODED,
                                                   _("Ctrl + Alt + Num (keypad)")));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" Drag"),
                                                   _("Moves the window."),
                                                   shortcut::OptionType::COMPIZ_MOUSE,
                                                   MOVE_PLUGIN_NAME,
                                                   MOVE_OPTION_INITIATE_BUTTON));

  hints.push_back(std::make_shared<shortcut::Hint>(windows, "", _(" Drag"),
                                                   _("Resizes the window."),
                                                   shortcut::OptionType::COMPIZ_MOUSE,
                                                   RESIZE_PLUGIN_NAME,
                                                   RESIZE_OPTION_INITIATE_BUTTON));
}

}
}
