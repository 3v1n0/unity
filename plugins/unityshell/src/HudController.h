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

#ifndef UNITY_HUD_CONTROLLER_H_
#define UNITY_HUD_CONTROLLER_H_

#include <memory>

#include <gdk/gdk.h>
#include <UnityCore/Hud.h>

#include <NuxCore/Property.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "HudView.h"
#include "UBusWrapper.h"

namespace unity
{
namespace hud
{

class Controller : public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller();
  ~Controller();

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

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void EnsureHud();
  void SetupWindow();
  void SetupHudView();
  void RegisterUBusInterests();
  void SetIcon(std::string const& icon_name);

  int GetTargetMonitor();
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
  static gboolean OnViewShowHideFrame(Controller* self);

  static void OnWindowConfigure(int width, int height, nux::Geometry& geo, void* data);

  void OnQueriesFinished(Hud::Queries queries);

private:
  UBusManager ubus;
  Hud hud_service_;
  nux::BaseWindow* window_;
  bool visible_;
  bool need_show_;

  guint timeline_id_;
  float last_opacity_;
  gint64 start_time_;

  View* view_;
  guint ensure_id_;
  std::string focused_app_icon_;
  nux::Layout* layout_;
  uint monitor_index_;
  guint type_wait_handle_;
  std::string last_search_;
};


}
}
#endif
