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

#include <UnityCore/ConnectionManager.h>
#include <UnityCore/GLibDBusServer.h>
#include <UnityCore/GLibSignal.h>

#include <NuxCore/Animation.h>
#include <NuxCore/Property.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/Nux.h>

#include "DashView.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/ResizingBaseWindow.h"

namespace unity
{
namespace dash
{

class Controller : public unity::debug::Introspectable, public sigc::trackable
{
public:
  typedef std::shared_ptr<Controller> Ptr;
  typedef std::function<ResizingBaseWindow*()> WindowCreator;

  Controller(WindowCreator const& create_window = nullptr);

  nux::BaseWindow* window() const;

  bool CheckShortcutActivation(const char* key_string);
  std::vector<char> GetAllShortcuts();

  nux::Property<bool> use_primary;

  sigc::signal<void> on_realize;

  void HideDash();
  void QuicklyHideDash();
  bool ShowDash();

  void ReFocusKeyInput();

  bool IsVisible() const;
  bool IsCommandLensOpen() const;
  nux::Geometry GetInputWindowGeometry();
  nux::ObjectPtr<DashView> const& Dash() const;

  int Monitor() const;

protected:
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

private:
  void EnsureDash();
  void SetupWindow();
  void SetupDashView();
  void RegisterUBusInterests();

  nux::Geometry GetIdealWindowGeometry();
  int GetIdealMonitor();
  void OnMonitorChanged(int primary, std::vector<nux::Geometry> const&);
  void UpdateDashPosition();
  void Relayout();

  void OnMouseDownOutsideWindow(int x, int y, unsigned long bflags, unsigned long kflags);
  void OnExternalShowDash(GVariant* variant);
  void OnExternalHideDash(GVariant* variant);
  void OnActivateRequest(GVariant* variant);

  void FocusWindow();

  void StartShowHideTimeline();
  void OnViewShowHideFrame(double progress);

  static void OnWindowConfigure(int width, int height, nux::Geometry& geo, void* data);

private:
  WindowCreator create_window_;
  nux::ObjectPtr<ResizingBaseWindow> window_;
  nux::ObjectPtr<DashView> view_;
  int monitor_;
  bool visible_;

  connection::Wrapper screen_ungrabbed_slot_;
  connection::Wrapper form_factor_changed_;
  glib::DBusServer dbus_server_;
  glib::TimeoutSeconds ensure_timeout_;
  glib::Source::UniquePtr grab_wait_;
  nux::animation::AnimateValue<double> timeline_animator_;
  UBusManager ubus_manager_;
};


}
}
#endif
