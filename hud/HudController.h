/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef UNITYSHELL_HUD_CONTROLLER_H
#define UNITYSHELL_HUD_CONTROLLER_H

#include <functional>
#include <memory>

#include <gdk/gdk.h>
#include <UnityCore/Hud.h>
#include <UnityCore/GLibSignal.h>

#include <NuxCore/Property.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>

#include "unity-shared/Animator.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/ResizingBaseWindow.h"
#include "HudView.h"

namespace unity
{
namespace hud
{

class Controller : public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller(std::function<AbstractView*(void)> const& function = []() { return new View; });

  nux::BaseWindow* window() const;

  nux::Property<int> launcher_width;
  nux::Property<int> icon_size;
  nux::Property<int> tile_size;
  nux::Property<bool> launcher_locked_out;
  nux::Property<bool> multiple_launchers;

  void ShowHideHud();
  void ShowHud();
  void HideHud(bool restore_focus = true);
  bool IsVisible();

  nux::Geometry GetInputWindowGeometry();

protected:
  // Introspectable
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void EnsureHud();
  void SetupWindow();
  void SetupHudView();
  void SetupRelayoutCallbacks();
  void RegisterUBusInterests();
  void SetIcon(std::string const& icon_name);

  int GetIdealMonitor();
  bool IsLockedToLauncher(int monitor);

  nux::Geometry GetIdealWindowGeometry();
  void Relayout();

  void OnMouseDownOutsideWindow(int x, int y, unsigned long bflags, unsigned long kflags);
  void OnScreenUngrabbed();
  void OnExternalShowHud(GVariant* variant);
  void OnExternalHideHud(GVariant* variant);
  void OnActivateRequest(GVariant* variant);

  void OnSearchChanged(std::string search_string);
  void OnSearchActivated(std::string search_string);
  void OnQueryActivated(Query::Ptr query);
  void OnQuerySelected(Query::Ptr query);

  void StartShowHideTimeline();
  void OnViewShowHideFrame(double progress);

  static void OnWindowConfigure(int width, int height, nux::Geometry& geo, void* data);

  void OnQueriesFinished(Hud::Queries queries);

private:
  nux::ObjectPtr<ResizingBaseWindow> window_;
  UBusManager ubus;
  glib::SignalManager sig_manager_;
  Hud hud_service_;
  bool visible_;
  bool need_show_;

  Animator timeline_animator_;

  AbstractView* view_;
  std::string focused_app_icon_;
  nux::Layout* layout_;
  uint monitor_index_;
  std::string last_search_;

  std::function<AbstractView*(void)> view_function_;
};

} // namespace hud
} // namespace unity

#endif // UNITYSHELL_HUD_CONTROLLER_H
