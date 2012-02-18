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

#ifndef UNITY_DASH_CONTROLLER_H_
#define UNITY_DASH_CONTROLLER_H_

#include <memory>

#include <gdk/gdk.h>
#include <UnityCore/GLibSignal.h>

#include <NuxCore/Property.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "DashView.h"
#include "Introspectable.h"
#include "UBusWrapper.h"

namespace unity
{
namespace dash
{

class Controller : public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller();
  ~Controller();

  nux::BaseWindow* window() const;

  gboolean CheckShortcutActivation(const char* key_string);
  std::vector<char> GetAllShortcuts();

  nux::Property<int> launcher_width;

  sigc::signal<void> on_realize;

protected:
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

private:
  void EnsureDash();
  void SetupWindow();
  void SetupDashView();
  void SetupRelayoutCallbacks();
  void RegisterUBusInterests();

  nux::Geometry GetIdealWindowGeometry();
  void Relayout(GdkScreen*screen=NULL);

  void OnMouseDownOutsideWindow(int x, int y, unsigned long bflags, unsigned long kflags);
  void OnScreenUngrabbed();
  void OnExternalShowDash(GVariant* variant);
  void OnExternalHideDash(GVariant* variant);
  void OnActivateRequest(GVariant* variant);

  void ShowDash();
  void HideDash(bool restore_focus = true);

  void StartShowHideTimeline();
  static gboolean OnViewShowHideFrame(Controller* self);

  static void OnWindowConfigure(int width, int height, nux::Geometry& geo, void* data);

private:
  glib::SignalManager sig_manager_;
  UBusManager ubus_manager_;

  nux::BaseWindow* window_;
  bool visible_;
  bool need_show_;

  guint timeline_id_;
  float last_opacity_;
  gint64 start_time_;

  DashView* view_;
  guint ensure_id_;
  sigc::connection screen_ungrabbed_slot_;
};


}
}
#endif
