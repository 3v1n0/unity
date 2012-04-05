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
 
#include <dbus/dbus-glib.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>
#include <Nux/Nux.h>
#include <Nux/WindowThread.h>

#include "BackgroundEffectHelper.h"
#include "MockShortcutHint.h"
#include "ShortcutController.h"

using namespace unity;

static shortcut::Controller::Ptr controller;

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  std::list<shortcut::AbstractHint*> hints;
  
  // Launcher...
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", _(" (Press)"), _("Open Launcher, displays shortcuts."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher" ));
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", "", _("Open Launcher keyboard navigation mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "keyboard_focus"));
  // FIXME: Implement it...
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", "", _("Switch application via Launcher."), shortcut::HARDCODED_OPTION, "Super + Tab"));
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", _(" + 1 to 9"), _("Same as clicking on a Launcher icon."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", _(" + Shift + 1 to 9"), _("Open a new window of the app."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints.push_back(new shortcut::MockHint(_("Launcher"), "", " + T", _("Open the Trash."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")); 

  // Dash...
  hints.push_back(new shortcut::MockHint(_("Dash"), "", _(" (Tap)"), _("Open the Dash Home."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  // These are not really hardcoded...
  hints.push_back(new shortcut::MockHint(_("Dash"), "", " + A", _("Open the Dash App Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", " + F", _("Open the Dash Files Lens."), shortcut::COMPIZ_KEY_OPTION,"unityshell", "show_launcher"));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", " + M", _("Open the Dash Music Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher"));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", "", _("Switches between Lenses."), shortcut::HARDCODED_OPTION, "Ctrl + Tab"));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", "", _("Moves the focus."), shortcut::HARDCODED_OPTION, _("Cursor Keys")));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", "", _("Open currently focused item."), shortcut::HARDCODED_OPTION, _("Enter / Return")));
  hints.push_back(new shortcut::MockHint(_("Dash"), "", "", _("'Run Command' mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "execute_command"));
 
  // Menu Bar
  // Is it really hard coded?
  hints.push_back(new shortcut::MockHint(_("Menu Bar"), "", "", _("Reveals application menu."), shortcut::HARDCODED_OPTION, "Alt"));
  hints.push_back(new shortcut::MockHint(_("Menu Bar"), "", "", _("Opens the indicator menu."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "panel_first_menu"));
  hints.push_back(new shortcut::MockHint(_("Menu Bar"), "", "", _("Moves focus between indicators."), shortcut::HARDCODED_OPTION, _("Cursor Left & Right")));

  // Switching
  hints.push_back(new shortcut::MockHint(_("Switching"), "", "", _("Switch between applications."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_forward"));
  hints.push_back(new shortcut::MockHint(_("Switching"), "", "", _("Switch windows of current application."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_next_window"));
  hints.push_back(new shortcut::MockHint(_("Switching"), "", "", _("Close window switch, return to app switch."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_detail_stop"));
  hints.push_back(new shortcut::MockHint(_("Switching"), "", "", _("Moves the foucs."), shortcut::HARDCODED_OPTION, _("Cursor Left & Right")));

  // Workspaces
  hints.push_back(new shortcut::MockHint(_("Workspaces"), "", "", _("Spread workspaces."), shortcut::COMPIZ_KEY_OPTION, "expo", "expo_key"));
  hints.push_back(new shortcut::MockHint(_("Workspaces"), "", "", _("Switch workspaces."), shortcut::HARDCODED_OPTION, _("Cursor Keys")));
  //hints.push_back(new shortcut::MockHint(_("Workspaces"), "", "", _("Move focused window to other workspace."), ...)

  // Windows
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Spreads all windows in current workspace."), shortcut::COMPIZ_KEY_OPTION, "scale", "initiate_output_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Minimises all windows."), shortcut::COMPIZ_KEY_OPTION, "core", "show_desktop_key"));
  // I don't know if it is really hardcoded, but I can't find where this option is stored.
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Open window accessibility menu."), shortcut::HARDCODED_OPTION, "Alt+Space"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Maximises current window."), shortcut::COMPIZ_KEY_OPTION, "core", "maximize_window_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Un-maximises current window."), shortcut::COMPIZ_KEY_OPTION, "core", "unmaximize_window_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Minimises current window."), shortcut::COMPIZ_KEY_OPTION, "core", "minimize_window_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Resizes current window."), shortcut::COMPIZ_KEY_OPTION, "resize", "initiate_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Closes current window."), shortcut::COMPIZ_KEY_OPTION, "core", "close_window_key"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Places window in corresponding positions."), shortcut::HARDCODED_OPTION, "Ctrl + Alt + Num"));
  hints.push_back(new shortcut::MockHint(_("Windows"), "", "", _("Move window."), shortcut::COMPIZ_KEY_OPTION, "move", "initiate_key"));
      
  controller.reset(new shortcut::Controller(hints));
  controller->SetWorkspace(nux::Geometry(25, 17, 1200 - 50, 720 - 35));
  controller->Show();
}

int main(int argc, char** argv)
{
  g_type_init();
  //g_thread_init(NULL);
  gtk_init(&argc, &argv);

  dbus_g_thread_init();

  nux::NuxInitialize(0);
  
  BackgroundEffectHelper::blur_type = BLUR_NONE;
  nux::WindowThread* wt = nux::CreateGUIThread(TEXT("Unity Shortcut Hint Overlay"), 1200, 720, 0, &ThreadWidgetInit, 0);

  wt->Run(NULL);
  delete wt;
  return 0;
}
