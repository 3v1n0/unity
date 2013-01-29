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
#include <Nux/NuxTimerTickSource.h>
#include <Nux/WindowThread.h>
#include <NuxCore/AnimationController.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "BaseWindowRaiserImp.h"
#include "MockShortcutHint.h"
#include "ShortcutController.h"
#include "unity-shared/UnitySettings.h"

using namespace unity;

namespace unity
{
namespace shortcut
{
struct StandaloneController : Controller
{
  StandaloneController(BaseWindowRaiser::Ptr const& raiser, AbstractModeller::Ptr const& modeller)
    : Controller(raiser, modeller)
  {}

  nux::Point GetOffsetPerMonitor(int monitor) override
  {
    return nux::Point();
  }
};
}
}

struct StandaloneModeller : shortcut::AbstractModeller
{
  StandaloneModeller()
  {
    BuildFullModel();

    model_switcher.reset(new glib::TimeoutSeconds(5, [this] {
      static bool toggle = false;
      if (toggle) BuildFullModel(); else BuildLightModel();
      toggle = !toggle;
      return true;
    }));
  }

  void BuildFullModel()
  {
    std::list<shortcut::AbstractHint::Ptr> hints;

    // Launcher...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" (Hold)"),
                                                                                   _("Opens the Launcher, displays shortcuts."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher" )));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "",
                                                                                   _("Opens Launcher keyboard navigation mode."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "keyboard_focus")));

    // FIXME: Implemstd::shared_ptr<shortcut::AbstractHint>(ent it...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "",
                                                                                   _("Switches applications via the Launcher."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   "Super + Tab")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + 1 to 9"),
                                                                                   _("Same as clicking on a Launcher icon."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" + Shift + 1 to 9"),
                                                                                   _("Opens a new window in the app."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", " + T",
                                                                                   _("Opens the Trash."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    // Dash...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", _(" (Tap)"),
                                                                                   _("Opens the Dash Home."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    // These are notstd::shared_ptr<shortcut::AbstractHint>( really hardcoded...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + A",
                                                                                   _("Opens the Dash App Lens."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + F",
                                                                                   _("Opens the Dash Files Lens."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", " + M",
                                                                                   _("Opens the Dash Music Lens."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                   _("Switches between Lenses."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   "Ctrl + Tab")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                   _("Moves the focus."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   _("Arrow Keys"))));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", "",
                                                                                   _("Opens the currently focused item."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   _("Enter"))));

    // Hud Menu Bar
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", _(" (Tap)"),
                                                                                    _("Opens the HUD."),
                                                                                    shortcut::OptionType::COMPIZ_KEY,
                                                                                    "unityshell",
                                                                                    "show_hud")));

    // Is it really std::shared_ptr<shortcut::AbstractHint>(hard coded?
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", _(" (Hold)"),
                                                                                   _("Reveals the application menu."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   "Alt")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", "",
                                                                                   _("Opens the indicator menu."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "panel_first_menu")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", "",
                                                                                   _("Moves focus between indicators."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   _("Cursor Left or Right"))));

    // Switching
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                   _("Switches between applications."),
                                                                                  shortcut::OptionType::COMPIZ_KEY,
                                                                                  "unityshell",
                                                                                  "alt_tab_forward")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                   _("Switches windows of current applications."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "alt_tab_next_window")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                   _("Moves the focus."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   _("Cursor Left or Right"))));

    // Workspaces
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "",
                                                                                   _("Switches between workspaces."),
                                                                                    shortcut::OptionType::COMPIZ_KEY,
                                                                                   "expo",
                                                                                   "expo_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Workspaces"), "", "",
                                                                                   _("Switches workspaces."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   _("Arrow Keys"))));

    //hints.push_bacstd::shared_ptr<shortcut::AbstractHint>(k(new shortcut::MockHint(_("Workspaces"), "", "", _("Move focused window to other workspace."), ...)

    // Windows
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Spreads all windows in the current workspace."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "scale",
                                                                                   "initiate_output_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Minimises all windows."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "core",
                                                                                   "show_desktop_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Maximises the current window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "core",
                                                                                   "maximize_window_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Restores or minimises the current window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "core",
                                                                                   "unmaximize_or_minimize_window_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", _(" or Right"),
                                                                                   _("Semi-maximises the current window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "grid",
                                                                                   "put_left_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Closes the current window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "core",
                                                                                   "close_window_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Opens the window accessibility menu."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "core",
                                                                                   "window_menu_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Places the window in corresponding position."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   "Ctrl + Alt + Num")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Moves the window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "move",
                                                                                   "initiate_key")));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Resizes the current window."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "resize",
                                                                                   "initiate_key")));
    model = std::make_shared<shortcut::Model>(hints);
    model_changed.emit(model);
  }

  void BuildLightModel()
  {
    std::list<shortcut::AbstractHint::Ptr> hints;

    // Launcher...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", _(" (Hold)"),
                                                                                   _("Opens the Launcher, displays shortcuts."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher" )));

    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Launcher"), "", "",
                                                                                   _("Opens Launcher keyboard navigation mode."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "keyboard_focus")));

    // Dash...
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Dash"), "", _(" (Tap)"),
                                                                                   _("Opens the Dash Home."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "unityshell",
                                                                                   "show_launcher")));

    // Is it really std::shared_ptr<shortcut::AbstractHint>(hard coded?
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("HUD & Menu Bar"), "", _(" (Hold)"),
                                                                                   _("Reveals the application menu."),
                                                                                   shortcut::OptionType::HARDCODED,
                                                                                   "Alt")));

    // Switching
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Switching"), "", "",
                                                                                   _("Switches between applications."),
                                                                                  shortcut::OptionType::COMPIZ_KEY,
                                                                                  "unityshell",
                                                                                  "alt_tab_forward")));


    // Windows
    hints.push_back(std::shared_ptr<shortcut::AbstractHint>(new shortcut::MockHint(_("Windows"), "", "",
                                                                                   _("Spreads all windows in the current workspace."),
                                                                                   shortcut::OptionType::COMPIZ_KEY,
                                                                                   "scale",
                                                                                   "initiate_output_key")));

    model = std::make_shared<shortcut::Model>(hints);
    model_changed.emit(model);
  }

  shortcut::Model::Ptr GetCurrentModel() const
  {
    return model;
  }

  shortcut::Model::Ptr model;
  glib::Source::UniquePtr model_switcher;
};

struct ShortcutsWindow
{
  ShortcutsWindow()
    : wt(nux::CreateGUIThread("Unity Shortcut Hint Overlay", 1024, 768, 0, &ShortcutsWindow::ThreadWidgetInit, this))
    , animation_controller(tick_source)
  {}

  void Show()
  {
    wt->Run(nullptr);
  }

private:
  void Init();

  static void ThreadWidgetInit(nux::NThread* thread, void* self)
  {
    static_cast<ShortcutsWindow*>(self)->Init();
  }

  unity::Settings settings;
  std::shared_ptr<nux::WindowThread> wt;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller;
  shortcut::Controller::Ptr controller;
};

void ShortcutsWindow::Init()
{
  BackgroundEffectHelper::blur_type = BLUR_NONE;
  auto base_window_raiser_ = std::make_shared<shortcut::BaseWindowRaiserImp>();
  auto modeller = std::make_shared<StandaloneModeller>();
  controller = std::make_shared<shortcut::StandaloneController>(base_window_raiser_, modeller);
  controller->Show();
}

int main(int argc, char** argv)
{
  gtk_init(&argc, &argv);

  nux::NuxInitialize(0);
  ShortcutsWindow().Show();

  return 0;
}
