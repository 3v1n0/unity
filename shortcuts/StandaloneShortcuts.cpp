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

#include "unity-shared/BackgroundEffectHelper.h"
#include "MockShortcutHint.h"
#include "ShortcutController.h"

using namespace unity;

static shortcut::Controller::Ptr controller;

void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{
  std::list<std::shared_ptr<shortcut::AbstractHint>> hints;

  // Launcher...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" (Hold)"), _("Open Launcher, displays shortcuts."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher" )));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "", _("Open Launcher keyboard navigation mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "keyboard_focus")));
  // FIXME: Implemstd::shared_ptr<shortcut::AbstractHint>(ent it...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "", _("Switch applications via the Launcher."), shortcut::HARDCODED_OPTION, "Super + Tab")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + 1 to 9"), _("Same as clicking on a Launcher icon."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + Shift + 1 to 9"), _("Open a new window of the app."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", " + T", _("Open the Trash."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));

  // Dash...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", _(" (Tap)"), _("Open the Dash Home."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));
  // These are notstd::shared_ptr<shortcut::AbstractHint>( really hardcoded...
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + A", _("Open the Dash App Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + F", _("Open the Dash Files Lens."), shortcut::COMPIZ_KEY_OPTION,"unityshell", "show_launcher")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + M", _("Open the Dash Music Lens."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "show_launcher")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "", _("Switch between Lenses."), shortcut::HARDCODED_OPTION, "Ctrl + Tab")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "", _("Move the focus."), shortcut::HARDCODED_OPTION, _("Arrow Keys"))));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "", _("Open currently focused item."), shortcut::HARDCODED_OPTION, _("Enter"))));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "", _("'Run Command' mode."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "execute_command")));

  // Menu Bar
  // Is it really std::shared_ptr<shortcut::AbstractHint>(hard coded?
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Menu Bar"), "", "", _("Reveals application menu."), shortcut::HARDCODED_OPTION, "Alt")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Menu Bar"), "", "", _("Opens the indicator menu."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "panel_first_menu")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Menu Bar"), "", "", _("Moves focus between indicators."), shortcut::HARDCODED_OPTION, _("Left & Right"))));

  // Switching
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "", _("Switch between applications."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_forward")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "", _("Switch windows of current application."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_next_window")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "", _("Close window switch, return to app switch."), shortcut::COMPIZ_KEY_OPTION, "unityshell", "alt_tab_detail_stop")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "", _("Move the foucs."), shortcut::HARDCODED_OPTION, _("Left & Right"))));

  // Workspaces
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "", _("Spread workspaces."), shortcut::COMPIZ_KEY_OPTION, "expo", "expo_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "", _("Switch workspaces."), shortcut::HARDCODED_OPTION, _("Arrow Keys"))));
  //hints.push_bacstd::shared_ptr<shortcut::AbstractHint>(k(new shortcut::MockHint(_("Workspaces"), "", "", _("Move focused window to other workspace."), ...)

  // Windows
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Spreads all windows in current workspace."), shortcut::COMPIZ_KEY_OPTION, "scale", "initiate_output_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Minimize all windows."), shortcut::COMPIZ_KEY_OPTION, "core", "show_desktop_key")));
  // I don't know std::shared_ptr<shortcut::AbstractHint>(if it is really hardcoded, but I can't find where this option is stored.
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Open the window accessibility menu."), shortcut::HARDCODED_OPTION, "Alt+Space")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Maximize current window."), shortcut::COMPIZ_KEY_OPTION, "core", "maximize_window_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Un-maximize current window."), shortcut::COMPIZ_KEY_OPTION, "core", "unmaximize_window_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Minimize current window."), shortcut::COMPIZ_KEY_OPTION, "core", "minimize_window_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Resize current window."), shortcut::COMPIZ_KEY_OPTION, "resize", "initiate_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Close current window."), shortcut::COMPIZ_KEY_OPTION, "core", "close_window_key")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Place window in corresponding position."), shortcut::HARDCODED_OPTION, "Ctrl + Alt + Num")));
  hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "", _("Move window."), shortcut::COMPIZ_KEY_OPTION, "move", "initiate_key")));

  controller.reset(new shortcut::Controller(hints));
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
