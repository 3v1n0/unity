// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2010-2012 Canonical Ltd
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

#include "DashController.h"

#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include <UnityCore/GLibWrapper.h>
#include "UnityCore/GSettingsScopes.h"

#include "ApplicationStarterImp.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/WindowManager.h"


namespace unity
{
namespace dash
{
DECLARE_LOGGER(logger, "unity.dash.controller");

const char* window_title = "unity-dash";

namespace
{
const unsigned int PRELOAD_TIMEOUT_LENGTH = 40;

namespace dbus
{
const std::string PATH = "/com/canonical/Unity/Dash";
const std::string INTROSPECTION =\
  "<node>"
  "  <interface name='com.canonical.Unity.Dash'>"
  ""
  "    <method name='HideDash'>"
  "    </method>"
  ""
  "  </interface>"
  "</node>";
}
}

Controller::Controller(Controller::WindowCreator const& create_window)
  : launcher_width(64)
  , use_primary(false)
  , create_window_(create_window)
  , monitor_(0)
  , visible_(false)
  , need_show_(false)
  , view_(nullptr)
  , ensure_timeout_(PRELOAD_TIMEOUT_LENGTH)
  , timeline_animator_(90)
{
  RegisterUBusInterests();

  ensure_timeout_.Run([&]() { EnsureDash(); return false; });
  timeline_animator_.updated.connect(sigc::mem_fun(this, &Controller::OnViewShowHideFrame));

  // As a default. the create_window_ function should just create a base window.
  if (create_window_ == nullptr)
  {
    create_window_ = [&]() {
      return new ResizingBaseWindow(dash::window_title,
                                    [this](nux::Geometry const& geo) {
                                      if (view_)
                                        return GetInputWindowGeometry();
                                      return geo;
                                    });
    };
  }

  SetupWindow();
  UScreen::GetDefault()->changed.connect([&] (int, std::vector<nux::Geometry>&) { Relayout(true); });

  Settings::Instance().form_factor.changed.connect([this](FormFactor)
  {
    if (window_ && view_ && visible_)
    {
      // Relayout here so the input window size updates.
      Relayout();

      window_->PushToFront();
      window_->SetInputFocus();
      nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
    }
  });

  auto spread_cb = sigc::bind(sigc::mem_fun(this, &Controller::HideDash), true);
  WindowManager::Default().initiate_spread.connect(spread_cb);

  dbus_server_.AddObjects(dbus::INTROSPECTION, dbus::PATH);
  dbus_server_.GetObjects().front()->SetMethodsCallsHandler([this] (std::string const& method, GVariant*) {
    if (method == "HideDash")
      HideDash();

    return static_cast<GVariant*>(nullptr);
  });
}

void Controller::SetupWindow()
{
  window_ = create_window_();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&Controller::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
  /* FIXME - first time we load our windows there is a race that causes the input
   * window not to actually get input, this side steps that by causing an input window
   * show and hide before we really need it. */
    WindowManager& wm = WindowManager::Default();
    wm.SaveInputFocus();
    window_->EnableInputWindow(true, dash::window_title, true, false);
    window_->EnableInputWindow(false, dash::window_title, true, false);
    wm.RestoreInputFocus();
  }
}

void Controller::SetupDashView()
{
  view_ = new DashView(std::make_shared<GSettingsScopes>(), std::make_shared<ApplicationStarterImp>());
  AddChild(view_);

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(view_, 1);
  layout->SetContentDistribution(nux::MAJOR_POSITION_START);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  window_->SetLayout(layout);

  window_->UpdateInputWindowGeometry();
}

void Controller::RegisterUBusInterests()
{
  ubus_manager_.RegisterInterest(UBUS_DASH_EXTERNAL_ACTIVATION,
                                 sigc::mem_fun(this, &Controller::OnExternalShowDash));
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST,
                                 sigc::mem_fun(this, &Controller::OnExternalHideDash));
  ubus_manager_.RegisterInterest(UBUS_DASH_ABOUT_TO_SHOW,
                                 [&] (GVariant*) { EnsureDash(); });
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [&] (GVariant *data)
  {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    int width = 0;
    int height = 0;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

    // hide if something else is coming up
    if (overlay_identity.Str() != "dash")
    {
      HideDash(true);
    }
  });

}

void Controller::EnsureDash()
{
  LOG_DEBUG(logger) << "Initializing Dash";
  if (!window_)
    SetupWindow();

  if (!view_)
  {
    SetupDashView();
    Relayout();
    ensure_timeout_.Remove();

    on_realize.emit();
  }
}

nux::BaseWindow* Controller::window() const
{
  return window_.GetPointer();
}

// We update the @geo that's sent in with our desired width and height
void Controller::OnWindowConfigure(int window_width, int window_height,
                                       nux::Geometry& geo, void* data)
{
  Controller* self = static_cast<Controller*>(data);
  geo = self->GetIdealWindowGeometry();
}

int Controller::GetIdealMonitor()
{
  UScreen *uscreen = UScreen::GetDefault();
  int primary_monitor;
  if (window_->IsVisible())
    primary_monitor = monitor_;
  else if (use_primary)
    primary_monitor = uscreen->GetPrimaryMonitor();
  else
    primary_monitor = uscreen->GetMonitorWithMouse();

  return primary_monitor;
}

nux::Geometry Controller::GetIdealWindowGeometry()
{
  UScreen *uscreen = UScreen::GetDefault();
  auto monitor_geo = uscreen->GetMonitorGeometry(GetIdealMonitor());

  // We want to cover as much of the screen as possible to grab any mouse events outside
  // of our window
  return nux::Geometry (monitor_geo.x + launcher_width,
                        monitor_geo.y,
                        monitor_geo.width - launcher_width,
                        monitor_geo.height);
}

void Controller::Relayout(bool check_monitor)
{
  EnsureDash();

  if (check_monitor)
  {
    monitor_ = CLAMP(GetIdealMonitor(), 0, static_cast<int>(UScreen::GetDefault()->GetMonitors().size()-1));
  }

  nux::Geometry geo = GetIdealWindowGeometry();
  view_->Relayout();
  window_->SetGeometry(geo);
  panel::Style &panel_style = panel::Style::Instance();
  view_->SetMonitorOffset(launcher_width, panel_style.panel_height);
}

void Controller::OnMouseDownOutsideWindow(int x, int y,
                                          unsigned long bflags, unsigned long kflags)
{
  HideDash();
}

void Controller::OnScreenUngrabbed()
{
  LOG_DEBUG(logger) << "On Screen Ungrabbed called";
  if (need_show_)
  {
    EnsureDash();
    ShowDash();
  }
}

void Controller::OnExternalShowDash(GVariant* variant)
{
  EnsureDash();
  visible_ ? HideDash() : ShowDash();
}

void Controller::OnExternalHideDash(GVariant* variant)
{
  EnsureDash();

  if (variant)
  {
    HideDash(g_variant_get_boolean(variant));
  }
  else
  {
    HideDash();
  }
}

void Controller::ShowDash()
{
  EnsureDash();
  WindowManager& wm = WindowManager::Default();
  // Don't want to show at the wrong time
  if (visible_ || wm.IsExpoActive() || wm.IsScaleActive())
    return;

  // We often need to wait for the mouse/keyboard to be available while a plugin
  // is finishing it's animations/cleaning up. In these cases, we patiently wait
  // for the screen to be available again before honouring the request.
  if (wm.IsScreenGrabbed())
  {
    screen_ungrabbed_slot_.disconnect();
    screen_ungrabbed_slot_ = wm.screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));
    need_show_ = true;
    return;
  }

  monitor_ = GetIdealMonitor();

  view_->AboutToShow();

  FocusWindow();

  need_show_ = false;
  visible_ = true;

  StartShowHideTimeline();

  nux::Geometry const& view_content_geo = view_->GetContentGeometry();

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_, view_content_geo.width, view_content_geo.height);
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
}

void Controller::FocusWindow()
{
  window_->ShowWindow(true);
  window_->PushToFront();
  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
    // in standalone (i.e. not embedded) mode, we do not need an input window. we are one.
    window_->EnableInputWindow(true, dash::window_title, true, false);
    // update the input window geometry. This causes the input window to match the actual size of the dash.
    window_->UpdateInputWindowGeometry();
  }
  window_->SetInputFocus();
  window_->QueueDraw();

  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
}

void Controller::QuicklyHideDash(bool restore)
{
  HideDash(restore);
  timeline_animator_.Stop();
  window_->ShowWindow(false);
}

void Controller::HideDash(bool restore)
{
  if (!visible_)
   return;

  screen_ungrabbed_slot_.disconnect();

  EnsureDash();

  view_->AboutToHide();

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, dash::window_title, true, false);
  visible_ = false;

  nux::GetWindowCompositor().SetKeyFocusArea(NULL,nux::KEY_NAV_NONE);

  if (restore)
    WindowManager::Default().RestoreInputFocus();

  StartShowHideTimeline();

  nux::Geometry const& view_content_geo = view_->GetContentGeometry();

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_, view_content_geo.width, view_content_geo.height);
  ubus_manager_.SendMessage(UBUS_OVERLAY_HIDDEN, info);
}

void Controller::StartShowHideTimeline()
{
  EnsureDash();

  if (timeline_animator_.CurrentState() == nux::animation::Animation::State::Running)
  {
    timeline_animator_.Reverse();
  }
  else
  {
    if (visible_)
      timeline_animator_.SetStartValue(0.0f).SetFinishValue(1.0f).Start();
    else
      timeline_animator_.SetStartValue(1.0f).SetFinishValue(0.0f).Start();
  }
}

void Controller::OnViewShowHideFrame(double opacity)
{
  window_->SetOpacity(opacity);

  if (opacity == 0.0f && !visible_)
  {
    window_->ShowWindow(false);
  }
}

void Controller::OnActivateRequest(GVariant* variant)
{
  EnsureDash();
  view_->OnActivateRequest(variant);
}

gboolean Controller::CheckShortcutActivation(const char* key_string)
{
  EnsureDash();
  std::string scope_id = view_->GetIdForShortcutActivation(std::string(key_string));
  if (scope_id != "")
  {
    WindowManager& wm = WindowManager::Default();
    if (wm.IsScaleActive())
      wm.TerminateScale();

    GVariant* args = g_variant_new("(sus)", scope_id.c_str(), dash::GOTO_DASH_URI, "");
    OnActivateRequest(args);
    g_variant_unref(args);
    return true;
  }
  return false;
}

std::vector<char> Controller::GetAllShortcuts()
{
  EnsureDash();
  return view_->GetAllShortcuts();
}

// Introspectable
std::string Controller::GetName() const
{
  return "DashController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder).add("visible", visible_)
                                  .add("ideal_monitor", GetIdealMonitor())
                                  .add("monitor", monitor_);
}

void Controller::ReFocusKeyInput()
{
  if (visible_)
  {
    window_->PushToFront();
    window_->SetInputFocus();
  }
}

bool Controller::IsVisible() const
{
  return visible_;
}

bool Controller::IsCommandLensOpen() const
{
  return visible_ && view_->IsCommandLensOpen();
}

nux::Geometry Controller::GetInputWindowGeometry()
{
  EnsureDash();
  dash::Style& style = dash::Style::Instance();
  nux::Geometry const& window_geo(window_->GetGeometry());
  nux::Geometry const& view_content_geo(view_->GetContentGeometry());

  nux::Geometry geo(window_geo.x, window_geo.y, view_content_geo.width, view_content_geo.height);
  geo.width += style.GetDashRightTileWidth();
  geo.height += style.GetDashBottomTileHeight();
  return geo;
}


}
}
