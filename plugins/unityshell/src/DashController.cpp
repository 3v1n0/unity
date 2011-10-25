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

DashController::DashController()
  : launcher_width(66)
  , panel_height(24)
  , window_(0)
  , visible_(false)
  , need_show_(false)
  , timeline_id_(0)
  , last_opacity_(0.0f)
  , start_time_(0)
{
  SetupRelayoutCallbacks();
  RegisterUBusInterests();

  PluginAdapter::Default()->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &DashController::OnScreenUngrabbed));

  ensure_id_ = g_timeout_add_seconds(60, [] (gpointer data) -> gboolean { static_cast<DashController*>(data)->EnsureDash(); return FALSE; }, this);

  DashSettings::GetDefault()->changed.connect([&]()
  {
    if (window_)
    {
      window_->PushToFront();
      window_->SetInputFocus();
      nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
    }
  });
}

DashController::~DashController()
{
  if (window_)
    window_->UnReference();
  window_ = 0;
  g_source_remove(timeline_id_);
  g_source_remove(ensure_id_);
}

void DashController::SetupWindow()
{
  window_ = new nux::BaseWindow("Dash");
  window_->SinkReference();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&DashController::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &DashController::OnMouseDownOutsideWindow));
}

void DashController::SetupDashView()
{
  view_ = new DashView();

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(view_, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);

  window_->SetLayout(layout);
}

void DashController::SetupRelayoutCallbacks()
{
  GdkScreen* screen = gdk_screen_get_default();

  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "monitors-changed", sigc::mem_fun(this, &DashController::Relayout)));
  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "size-changed", sigc::mem_fun(this, &DashController::Relayout)));
}

void DashController::RegisterUBusInterests()
{
  ubus_manager_.RegisterInterest(UBUS_DASH_EXTERNAL_ACTIVATION,
                                 sigc::mem_fun(this, &DashController::OnExternalShowDash));
  ubus_manager_.RegisterInterest(UBUS_PLACE_VIEW_CLOSE_REQUEST,
                                 sigc::mem_fun(this, &DashController::OnExternalHideDash));
  ubus_manager_.RegisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                 sigc::mem_fun(this, &DashController::OnActivateRequest));
  ubus_manager_.RegisterInterest(UBUS_DASH_ABOUT_TO_SHOW,
                                 [&] (GVariant*) { EnsureDash(); });
}

void DashController::EnsureDash()
{
  if (window_)
    return;

  LOG_DEBUG(logger) << "Initializing Dash";

  SetupWindow();
  SetupDashView();
  Relayout();
  ensure_id_ = 0;

  on_realize.emit();
}

nux::BaseWindow* DashController::window() const
{
  return window_;
}

// We update the @geo that's sent in with our desired width and height
void DashController::OnWindowConfigure(int window_width, int window_height,
                                       nux::Geometry& geo, void* data)
{
  DashController* self = static_cast<DashController*>(data);
  geo = self->GetIdealWindowGeometry();
}

nux::Geometry DashController::GetIdealWindowGeometry()
{
  UScreen *uscreen = UScreen::GetDefault();
  int primary_monitor = uscreen->GetPrimaryMonitor();
  auto monitor_geo = uscreen->GetMonitorGeometry(primary_monitor);

  // We want to cover as much of the screen as possible to grab any mouse events outside
  // of our window
  return nux::Geometry (monitor_geo.x + launcher_width,
                        monitor_geo.y + panel_height,
                        monitor_geo.width - launcher_width,
                        monitor_geo.height - panel_height);
}

void DashController::Relayout(GdkScreen*screen)
{
  EnsureDash();

  nux::Geometry geo = GetIdealWindowGeometry();
  window_->SetGeometry(geo);
  view_->Relayout();
}

void DashController::OnMouseDownOutsideWindow(int x, int y,
                                              unsigned long bflags, unsigned long kflags)
{
  HideDash();
}

void DashController::OnScreenUngrabbed()
{
  if (need_show_)
  {
    EnsureDash();
    ShowDash();
  }
}

void DashController::OnExternalShowDash(GVariant* variant)
{
  EnsureDash();
  visible_ ? HideDash() : ShowDash();
}

void DashController::OnExternalHideDash(GVariant* variant)
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

void DashController::ShowDash()
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
    need_show_ = true;
    return;
  }

  adaptor->saveInputFocus ();

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

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_SHOWN);
}

void DashController::HideDash(bool restore)
{
  if (!visible_)
   return;
 
  EnsureDash();

  view_->AboutToHide();

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Dash", true, false);
  visible_ = false;

  if (restore)
    PluginAdapter::Default ()->restoreInputFocus ();

  StartShowHideTimeline();

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_HIDDEN);
}

void DashController::StartShowHideTimeline()
{
  EnsureDash();

  if (timeline_id_)
    g_source_remove(timeline_id_);

  timeline_id_ = g_timeout_add(15, (GSourceFunc)DashController::OnViewShowHideFrame, this);
  last_opacity_ = window_->GetOpacity();
  start_time_ = g_get_monotonic_time();

}

gboolean DashController::OnViewShowHideFrame(DashController* self)
{
#define _LENGTH_ 90000
  float diff = g_get_monotonic_time() - self->start_time_;
  float progress = diff / (float)_LENGTH_;
  float last_opacity = self->last_opacity_;

  if (self->visible_)
  {
    self->window_->SetOpacity(last_opacity + ((1.0f - last_opacity) * progress));
  }
  else
  {
    self->window_->SetOpacity(last_opacity - (last_opacity * progress));
  }

  if (diff > _LENGTH_)
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

void DashController::OnActivateRequest(GVariant* variant)
{
  EnsureDash();
  ubus_manager_.UnregisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST);
  view_->OnActivateRequest(variant);
  ShowDash();
}

gboolean DashController::CheckShortcutActivation(const char* key_string)
{
  EnsureDash();
  std::string lens_id = view_->GetIdForShortcutActivation(std::string(key_string));
  if (lens_id != "")
  {
    OnActivateRequest(g_variant_new("(sus)", lens_id.c_str(), 0, ""));
    return true;
  }
  return false;
}

std::vector<char> DashController::GetAllShortcuts()
{
  EnsureDash();
  return view_->GetAllShortcuts();
}

// Introspectable
const gchar* DashController::GetName()
{
  return "DashController";
}

void DashController::AddProperties(GVariantBuilder* builder)
{}


}
}
