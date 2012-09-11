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
#include "unity-shared/Animator.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace dash
{

class Controller : public unity::debug::Introspectable
{
public:
  typedef std::shared_ptr<Controller> Ptr;

  Controller();

  nux::BaseWindow* window() const;

  gboolean CheckShortcutActivation(const char* key_string);
  std::vector<char> GetAllShortcuts();

  nux::Property<int> launcher_width;
  nux::Property<bool> use_primary;

  sigc::signal<void> on_realize;

  void HideDash(bool restore_focus = true);

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
  int GetIdealMonitor();
  void Relayout(GdkScreen*screen=NULL);

  void OnMouseDownOutsideWindow(int x, int y, unsigned long bflags, unsigned long kflags);
  void OnScreenUngrabbed();
  void OnExternalShowDash(GVariant* variant);
  void OnExternalHideDash(GVariant* variant);
  void OnActivateRequest(GVariant* variant);

  void ShowDash();

  void StartShowHideTimeline();
  void OnViewShowHideFrame(double progress);

  static void OnBusAcquired(GObject *obj, GAsyncResult *result, gpointer user_data);
  static void OnDBusMethodCall(GDBusConnection* connection, const gchar* sender,
                               const gchar* object_path, const gchar* interface_name,
                               const gchar* method_name, GVariant* parameters,
                               GDBusMethodInvocation* invocation, gpointer user_data);

  static void OnWindowConfigure(int width, int height, nux::Geometry& geo, void* data);

private:
  nux::ObjectPtr<nux::BaseWindow> window_;
  int monitor_;

  bool visible_;
  bool need_show_;
  DashView* view_;

  sigc::connection screen_ungrabbed_slot_;
  glib::SignalManager sig_manager_;
  glib::TimeoutSeconds ensure_timeout_;
  glib::SourceManager sources_;
  Animator timeline_animator_;
  UBusManager ubus_manager_;
  unsigned int dbus_owner_;
  static GDBusInterfaceVTable interface_vtable;
};


}
}
#endif
