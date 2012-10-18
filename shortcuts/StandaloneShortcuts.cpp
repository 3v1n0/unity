// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 */

#include "config.h"
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <Nux/Nux.h>
#include <Nux/WindowThread.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "BaseWindowRaiserImp.h"
#include "MockShortcutHint.h"
#include "ShortcutController.h"

using namespace unity;

static shortcut::Controller::Ptr controller;

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  std::list<std::shared_ptr<shortcut::AbstractHint>> hints;

  // Launcher...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" (Hold)"),
                                                                                 _("Opens the Launcher, displays shortcuts."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher" )));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "",
                                                                                 _("Opens Launcher keyboard navigation mode."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "keyboard_focus")));

  // FIXME: Implemstd::shared_ptr<shortcut::AbstractHint>(ent it...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "",
                                                                                 _("Switches applications via the Launcher."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 "Super + Tab")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + 1 to 9"),
                                                                                 _("Same as clicking on a Launcher icon."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + Shift + 1 to 9"),
                                                                                 _("Opens a new window in the app."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", " + T",
                                                                                 _("Opens the Trash."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  // Dash...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", _(" (Tap)"),
                                                                                 _("Opens the Dash Home."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  // These are notstd::shared_ptr<shortcut::AbstractHint>( really hardcoded...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + A",
                                                                                 _("Opens the Dash App Lens."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + F",
                                                                                 _("Opens the Dash Files Lens."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + M",
                                                                                 _("Opens the Dash Music Lens."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "show_launcher")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                 _("Switches between Lenses."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 "Ctrl + Tab")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                 _("Moves the focus."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 _("Arrow Keys"))));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                 _("Opens the currently focused item."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 _("Enter"))));

  // Hud Menu Bar
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", _(" (Tap)"),
                                                                                  _("Opens the HUD."),
                                                                                  shortcut::COMPIZ_KEY_OPTION,
                                                                                  "unityshell",
                                                                                  "show_hud")));

  // Is it really std::shared_ptr<shortcut::AbstractHint>(hard coded?
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", _(" (Hold)"),
                                                                                 _("Reveals the application menu."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 "Alt")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", "",
                                                                                 _("Opens the indicator menu."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "panel_first_menu")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", "",
                                                                                 _("Moves focus between indicators."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 _("Cursor Left or Right"))));

  // Switching
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                 _("Switches between applications."),
                                                                                shortcut::COMPIZ_KEY_OPTION,
                                                                                "unityshell",
                                                                                "alt_tab_forward")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                 _("Switches windows of current applications."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "unityshell",
                                                                                 "alt_tab_next_window")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                 _("Moves the focus."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 _("Cursor Left or Right"))));

  // Workspaces
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "",
                                                                                 _("Switches between workspaces."),
                                                                                  shortcut::COMPIZ_KEY_OPTION,
                                                                                 "expo",
                                                                                 "expo_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "",
                                                                                 _("Switches workspaces."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 _("Arrow Keys"))));

  //hints.push_bacstd::shared_ptr<shortcut::AbstractHint>(k(new shortcut::MockHint(_("Workspaces"), "", "", _("Move focused window to other workspace."), ...)

  // Windows
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Spreads all windows in the current workspace."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "scale",
                                                                                 "initiate_output_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Minimises all windows."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "core",
                                                                                 "show_desktop_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Maximises the current window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "core",
                                                                                 "maximize_window_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Restores or minimises the current window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "core",
                                                                                 "unmaximize_window_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", _(" or Right"),
                                                                                 _("Semi-maximises the current window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "grid",
                                                                                 "put_left_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Closes the current window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "core",
                                                                                 "close_window_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Opens the window accessibility menu."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "core",
                                                                                 "window_menu_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Places the window in corresponding position."),
                                                                                 shortcut::HARDCODED_OPTION,
                                                                                 "Ctrl + Alt + Num")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Moves the window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "move",
                                                                                 "initiate_key")));

  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                 _("Resizes the current window."),
                                                                                 shortcut::COMPIZ_KEY_OPTION,
                                                                                 "resize",
                                                                                 "initiate_key")));

  auto base_window_raiser_ = std::make_shared<shortcut::BaseWindowRaiserImp>();
  controller.reset(new shortcut::Controller(hints, base_window_raiser_));
  controller->Show();
}

int main(int argc, char** argv)
{
  g_type_init();
  gtk_init(&argc, &argv);

  nux::NuxInitialize(0);

  BackgroundEffectHelper::blur_type = BLUR_NONE;
  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Shortcut Hint Overlay"), 1200, 720, 0, &ThreadWidgetInit, 0);

  wt->Run(NULL);
  delete wt;
  return 0;
}
