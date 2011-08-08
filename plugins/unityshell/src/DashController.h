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
#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/BaseWindow.h>

#include "Introspectable.h"
#include "UBusWrapper.h"

namespace unity
{
namespace dash
{

class DashController : public unity::Introspectable
{
public:
  typedef std::shared_ptr<DashController> Ptr;

  DashController();
  ~DashController();

  nux::BaseWindow* window() const;

  nux::Property<int> launcher_width;
  nux::Property<int> panel_height;

protected:
  const gchar* GetName();
  void AddProperties(GVariantBuilder* builder);

private:
  void SetupWindow();
  void SetupRelayoutCallbacks();
  void RegisterUBusInterests();

  nux::Geometry GetIdealWindowGeometry();
  void Relayout(GdkScreen*screen=NULL);

  void OnMouseDownOutsideWindow(int x, int y, unsigned long bflags, unsigned long kflags);
  void OnScreenUngrabbed();
  void OnExternalShowDash(GVariant* variant);
  void OnExternalHideDash(GVariant* variant);

  void ShowDash();
  void HideDash();

  void StartShowHideTimeline();
  static gboolean OnViewShowHideFrame(DashController* self);

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
};


}
}
#endif
