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
#include "unity-shared/AnimationUtils.h"
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
const unsigned PRELOAD_TIMEOUT_LENGTH = 40;
const unsigned FADE_DURATION = 90;

namespace dbus
{
const std::string BUS_NAME = "com.canonical.Unity";
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
  : use_primary(false)
  , create_window_(create_window)
  , monitor_(0)
  , visible_(false)
  , dbus_server_(dbus::BUS_NAME)
  , ensure_timeout_(PRELOAD_TIMEOUT_LENGTH)
  , timeline_animator_(Settings::Instance().low_gfx() ? 0 : FADE_DURATION)
{
  RegisterUBusInterests();

  ensure_timeout_.Run([this]() { EnsureDash(); return false; });
  timeline_animator_.updated.connect(sigc::mem_fun(this, &Controller::OnViewShowHideFrame));

  // As a default. the create_window_ function should just create a base window.
  if (create_window_ == nullptr)
  {
    create_window_ = [this]() {
      return new ResizingBaseWindow(dash::window_title,
                                    [this](nux::Geometry const& geo) {
                                      if (view_)
                                        return GetInputWindowGeometry();
                                      return geo;
                                    });
    };
  }

  SetupWindow();
  UScreen::GetDefault()->changed.connect(sigc::mem_fun(this, &Controller::OnMonitorChanged));
  Settings::Instance().launcher_position.changed.connect(sigc::hide(sigc::mem_fun(this, &Controller::Relayout)));
  Settings::Instance().low_gfx.changed.connect(sigc::track_obj([this] (bool low_gfx) {
    timeline_animator_.SetDuration(low_gfx ? 0 : FADE_DURATION);
  }, *this));

  form_factor_changed_ = Settings::Instance().form_factor.changed.connect([this] (FormFactor)
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

  auto& wm = WindowManager::Default();
  wm.initiate_spread.connect(sigc::mem_fun(this, &Controller::HideDash));
  wm.screen_viewport_switch_started.connect(sigc::mem_fun(this, &Controller::HideDash));

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
  AddChild(view_.GetPointer());

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(view_.GetPointer(), 1);
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
                                 [this] (GVariant*) { EnsureDash(); });
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [this] (GVariant *data)
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
      HideDash();
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

int Controller::Monitor() const
{
  return monitor_;
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
  int launcher_size = unity::Settings::Instance().LauncherSize(monitor_);

  // We want to cover as much of the screen as possible to grab any mouse events outside
  // of our window
  if (Settings::Instance().launcher_position() == LauncherPosition::LEFT)
  {
    monitor_geo.x += launcher_size;
    monitor_geo.width -= launcher_size;
  }
  else
  {
    monitor_geo.height -= launcher_size;
  }

  return monitor_geo;
}

void Controller::OnMonitorChanged(int primary, std::vector<nux::Geometry> const& monitors)
{
  if (!visible_ || !window_ || !view_)
    return;

  monitor_ = std::min<int>(GetIdealMonitor(), monitors.size()-1);
  view_->SetMonitor(monitor_);
  Relayout();
}

void Controller::Relayout()
{
  EnsureDash();

  view_->Relayout();
  window_->SetGeometry(GetIdealWindowGeometry());
  UpdateDashPosition();
}

void Controller::UpdateDashPosition()
{
  auto launcher_position = Settings::Instance().launcher_position();
  int left_offset = 0;
  int top_offset = panel::Style::Instance().PanelHeight(monitor_);
  int launcher_size = unity::Settings::Instance().LauncherSize(monitor_);

  if (launcher_position == LauncherPosition::LEFT)
  {
    left_offset = launcher_size;
  }
  else if (launcher_position == LauncherPosition::BOTTOM &&
           Settings::Instance().form_factor() == FormFactor::DESKTOP)
  {
    auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor_);
    top_offset = monitor_geo.height - view_->GetContentGeometry().height - launcher_size;
  }

  view_->SetMonitorOffset(left_offset, top_offset);
}

void Controller::OnMouseDownOutsideWindow(int x, int y,
                                          unsigned long bflags, unsigned long kflags)
{
  HideDash();
}

void Controller::OnExternalShowDash(GVariant* variant)
{
  EnsureDash();

  if (!visible_)
    ShowDash();
  else
    HideDash();
}

void Controller::OnExternalHideDash(GVariant* variant)
{
  HideDash();
}

bool Controller::ShowDash()
{
  // Don't want to show at the wrong time
  if (visible_)
    return false;

  WindowManager& wm = WindowManager::Default();

  if (wm.IsExpoActive())
    wm.TerminateExpo();

  // We often need to wait for the mouse/keyboard to be available while a plugin
  // is finishing it's animations/cleaning up. In these cases, we patiently wait
  // for the screen to be available again before honouring the request.
  if (wm.IsScreenGrabbed())
  {
    screen_ungrabbed_slot_ = wm.screen_ungrabbed.connect([this] {
      grab_wait_.reset();
      ShowDash();
    });

    // Let's wait ungrab event for maximum a couple of seconds...
    grab_wait_.reset(new glib::TimeoutSeconds(2, [this] {
      screen_ungrabbed_slot_->disconnect();
      return false;
    }));

    return false;
  }

  screen_ungrabbed_slot_->disconnect();
  wm.SaveInputFocus();

  EnsureDash();
  monitor_ = GetIdealMonitor();
  view_->SetMonitor(monitor_);
  view_->AboutToShow();
  UpdateDashPosition();
  FocusWindow();

  visible_ = true;
  StartShowHideTimeline();

  nux::Geometry const& view_content_geo = view_->GetContentGeometry();

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_, view_content_geo.width, view_content_geo.height);
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
  return true;
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

void Controller::QuicklyHideDash()
{
  HideDash();
  timeline_animator_.Stop();
  window_->ShowWindow(false);
}

void Controller::HideDash()
{
  if (!visible_)
   return;

  EnsureDash();

  view_->AboutToHide();

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, dash::window_title, true, false);
  visible_ = false;

  auto& wc = nux::GetWindowCompositor();
  auto *key_focus_area = wc.GetKeyFocusArea();
  if (key_focus_area && key_focus_area->IsChildOf(view_.GetPointer()))
    wc.SetKeyFocusArea(nullptr, nux::KEY_NAV_NONE);

  WindowManager::Default().RestoreInputFocus();

  StartShowHideTimeline();

  nux::Geometry const& view_content_geo = view_->GetContentGeometry();

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_, view_content_geo.width, view_content_geo.height);
  ubus_manager_.SendMessage(UBUS_OVERLAY_HIDDEN, info);
}

void Controller::StartShowHideTimeline()
{
  EnsureDash();
  animation::StartOrReverseIf(timeline_animator_, visible_);
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

bool Controller::CheckShortcutActivation(const char* key_string)
{
  if (!key_string)
    return false;

  EnsureDash();
  std::string scope_id = view_->GetIdForShortcutActivation(key_string);
  if (!scope_id.empty())
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

void Controller::AddProperties(debug::IntrospectionData& introspection)
{
  introspection.add("visible", visible_)
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
  int launcher_size = Settings::Instance().LauncherSize(monitor_);
  auto const& monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(monitor_);
  dash::Style& style = dash::Style::Instance();
  nux::Geometry const& window_geo(window_->GetGeometry());
  nux::Geometry const& view_content_geo(view_->GetContentGeometry());

  nux::Geometry geo(window_geo.x, window_geo.y, view_content_geo.width, view_content_geo.height);

  if (Settings::Instance().form_factor() == FormFactor::DESKTOP)
  {
    geo.width += style.GetDashVerticalBorderWidth().CP(view_->scale());
    geo.height += style.GetDashHorizontalBorderHeight().CP(view_->scale());

    if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
      geo.y = monitor_geo.height - view_content_geo.height - launcher_size - style.GetDashHorizontalBorderHeight().CP(view_->scale());
  }
  else if (Settings::Instance().form_factor() == FormFactor::NETBOOK)
  {
    geo.height = monitor_geo.height;
    if (Settings::Instance().launcher_position() == LauncherPosition::BOTTOM)
      geo.height -= launcher_size;
  }

  return geo;
}

nux::ObjectPtr<DashView> const& Controller::Dash() const
{
  return view_;
}


}
}
