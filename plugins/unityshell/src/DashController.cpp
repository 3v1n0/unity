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

#include "DashController.h"

#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>

#include "DashSettings.h"
#include "PanelStyle.h"
#include "PluginAdapter.h"
#include "UBusMessages.h"
#include "UScreen.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.controller");
}

Controller::Controller()
  : launcher_width(64)
  , use_primary(false)
  , monitor_(0)
  , window_(0)
  , visible_(false)
  , need_show_(false)
  , timeline_id_(0)
  , last_opacity_(0.0f)
  , start_time_(0)
  , view_(nullptr)
{
  SetupRelayoutCallbacks();
  RegisterUBusInterests();

  ensure_id_ = g_timeout_add_seconds(60, [] (gpointer data) -> gboolean { static_cast<Controller*>(data)->EnsureDash(); return FALSE; }, this);

  SetupWindow();
  
  Settings::Instance().changed.connect([&]()
  {
    if (window_ && view_)
    {
      window_->PushToFront();
      window_->SetInputFocus();
      nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
    }
  });
}

Controller::~Controller()
{
  if (window_)
    window_->UnReference();
  window_ = 0;

  if (timeline_id_)
    g_source_remove(timeline_id_);

  if (ensure_id_)
    g_source_remove(ensure_id_);
}

void Controller::SetupWindow()
{
  window_ = new nux::BaseWindow("Dash");
  window_->SinkReference();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&Controller::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));
  
  /* FIXME - first time we load our windows there is a race that causes the input window not to actually get input, this side steps that by causing an input window show and hide before we really need it. */
  PluginAdapter::Default()->saveInputFocus ();
  window_->EnableInputWindow(true, "Dash", true, false);
  window_->EnableInputWindow(false, "Dash", true, false);
  PluginAdapter::Default()->restoreInputFocus ();
}

void Controller::SetupDashView()
{
  view_ = new DashView();
  AddChild(view_);

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(view_, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  window_->SetLayout(layout);
  ubus_manager_.UnregisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST);
}

void Controller::SetupRelayoutCallbacks()
{
  GdkScreen* screen = gdk_screen_get_default();

  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "monitors-changed", sigc::mem_fun(this, &Controller::Relayout)));
  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "size-changed", sigc::mem_fun(this, &Controller::Relayout)));
}

void Controller::RegisterUBusInterests()
{
  ubus_manager_.RegisterInterest(UBUS_DASH_EXTERNAL_ACTIVATION,
                                 sigc::mem_fun(this, &Controller::OnExternalShowDash));
  ubus_manager_.RegisterInterest(UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 sigc::mem_fun(this, &Controller::OnExternalHideDash));
  ubus_manager_.RegisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                 sigc::mem_fun(this, &Controller::OnActivateRequest));
  ubus_manager_.RegisterInterest(UBUS_DASH_ABOUT_TO_SHOW,
                                 [&] (GVariant*) { EnsureDash(); });
  ubus_manager_.RegisterInterest(UBUS_OVERLAY_SHOWN, [&] (GVariant *data) {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, &overlay_identity, &can_maximise, &overlay_monitor);

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
    ensure_id_ = 0;

    on_realize.emit();
  }
}

nux::BaseWindow* Controller::window() const
{
  return window_;
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
  if (use_primary)
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
  panel::Style &panel_style = panel::Style::Instance();
  return nux::Geometry (monitor_geo.x + launcher_width,
                        monitor_geo.y + panel_style.panel_height,
                        monitor_geo.width - launcher_width,
                        monitor_geo.height - panel_style.panel_height);
}

void Controller::Relayout(GdkScreen*screen)
{
  EnsureDash();

  nux::Geometry geo = GetIdealWindowGeometry();
  window_->SetGeometry(geo);
  view_->Relayout();
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

  PluginAdapter* adaptor = PluginAdapter::Default();
  // Don't want to show at the wrong time
  if (visible_ || adaptor->IsExpoActive() || adaptor->IsScaleActive())
    return;

  // We often need to wait for the mouse/keyboard to be available while a plugin
  // is finishing it's animations/cleaning up. In these cases, we patiently wait
  // for the screen to be available again before honouring the request.
  if (adaptor->IsScreenGrabbed())
  {
    screen_ungrabbed_slot_.disconnect();
    screen_ungrabbed_slot_ = PluginAdapter::Default()->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));
    need_show_ = true;
    return;
  }

  view_->AboutToShow();

  window_->ShowWindow(true);
  window_->PushToFront();
  window_->EnableInputWindow(true, "Dash", true, false);
  window_->SetInputFocus();
  window_->CaptureMouseDownAnyWhereElse(true);
  window_->QueueDraw();

  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());

  need_show_ = false;
  visible_ = true;

  StartShowHideTimeline();

  monitor_ = GetIdealMonitor();
  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_);
  ubus_manager_.SendMessage(UBUS_OVERLAY_SHOWN, info);
}

void Controller::HideDash(bool restore)
{
  if (!visible_)
   return;

  screen_ungrabbed_slot_.disconnect();

  EnsureDash();

  view_->AboutToHide();

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Dash", true, false);
  visible_ = false;

  nux::GetWindowCompositor().SetKeyFocusArea(NULL,nux::KEY_NAV_NONE);

  if (restore)
    PluginAdapter::Default ()->restoreInputFocus ();

  StartShowHideTimeline();

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "dash", TRUE, monitor_);
  ubus_manager_.SendMessage(UBUS_OVERLAY_HIDDEN, info);
}

void Controller::StartShowHideTimeline()
{
  EnsureDash();

  if (timeline_id_)
    g_source_remove(timeline_id_);

  timeline_id_ = g_timeout_add(15, (GSourceFunc)Controller::OnViewShowHideFrame, this);
  last_opacity_ = window_->GetOpacity();
  start_time_ = g_get_monotonic_time();

}

gboolean Controller::OnViewShowHideFrame(Controller* self)
{
  const float LENGTH = 90000.0f;
  float diff = g_get_monotonic_time() - self->start_time_;
  float progress = diff / LENGTH;
  float last_opacity = self->last_opacity_;

  if (self->visible_)
  {
    self->window_->SetOpacity(last_opacity + ((1.0f - last_opacity) * progress));
  }
  else
  {
    self->window_->SetOpacity(last_opacity - (last_opacity * progress));
  }

  if (diff > LENGTH)
  {
    self->timeline_id_ = 0;

    // Make sure the state is right
    self->window_->SetOpacity(self->visible_ ? 1.0f : 0.0f);
    if (!self->visible_)
    {
      self->window_->ShowWindow(false);
    }

    return FALSE;
  }

  return TRUE;
}

void Controller::OnActivateRequest(GVariant* variant)
{
  EnsureDash();
  view_->OnActivateRequest(variant);
}

gboolean Controller::CheckShortcutActivation(const char* key_string)
{
  EnsureDash();
  std::string lens_id = view_->GetIdForShortcutActivation(std::string(key_string));
  if (lens_id != "")
  {
    GVariant* args = g_variant_new("(sus)", lens_id.c_str(), dash::GOTO_DASH_URI, "");
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
                                  .add("monitor", monitor_);
}


}
}
