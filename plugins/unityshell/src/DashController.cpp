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

#include "NuxCore/Logger.h"
#include "Nux/HLayout.h"

#include "PluginAdapter.h"
#include "UBusMessages.h"

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
  SetupWindow();
  SetupDashView();
  SetupRelayoutCallbacks();
  RegisterUBusInterests();

  PluginAdapter::Default()->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &DashController::OnScreenUngrabbed));

  Relayout();
}

DashController::~DashController()
{
  window_->UnReference();
  g_source_remove(timeline_id_);
}

void DashController::SetupWindow()
{
  window_ = new nux::BaseWindow("Dash");
  window_->SinkReference();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&DashController::OnWindowConfigure, this);
  window_->EnableInputWindow(true, "Dash", false, false);
  window_->EnableInputWindow(false);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->SetFocused(true);
  window_->OnMouseDownOutsideArea.connect(sigc::mem_fun(this, &DashController::OnMouseDownOutsideWindow));
}

void DashController::SetupDashView()
{
  view_ = new DashView();

  nux::HLayout* layout = new nux::HLayout();
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
  GdkScreen* screen = gdk_screen_get_default();
  int primary_monitor = gdk_screen_get_primary_monitor(screen);

  GdkRectangle monitor_geo = { 0 };
  gdk_screen_get_monitor_geometry(screen, primary_monitor, &monitor_geo);
  
  // We want to cover as much of the screen as possible to grab any mouse events outside
  // of our window
  return nux::Geometry (monitor_geo.x + launcher_width,
                        monitor_geo.y + panel_height,
                        monitor_geo.width - launcher_width,
                        monitor_geo.height - panel_height);
}

void DashController::Relayout(GdkScreen*screen)
{
  nux::Geometry geo = GetIdealWindowGeometry();
  window_->SetGeometry(geo);
}

void DashController::OnMouseDownOutsideWindow(int x, int y,
                                              unsigned long bflags, unsigned long kflags)
{
  HideDash();
}

void DashController::OnScreenUngrabbed()
{
  if (need_show_)
    ShowDash();
}

void DashController::OnExternalShowDash(GVariant* variant)
{
  visible_ ? HideDash() : ShowDash();
}

void DashController::OnExternalHideDash(GVariant* variant)
{
  HideDash();
}

void DashController::ShowDash()
{
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

  view_->AboutToShow();

  window_->ShowWindow(true, true);
  window_->PushToFront();
  window_->EnableInputWindow(true);
  window_->CaptureMouseDownAnyWhereElse(true);
  window_->QueueDraw();
 
  //nux::GetWindowCompositor().SetKeyFocusArea(_view->GetDefaultFocus());

  need_show_ = false;
  visible_ = true;

  StartShowHideTimeline();

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_SHOWN);
}

void DashController::HideDash()
{
  if (!visible_)
   return;

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->ForceStopFocus(1, 1);
  window_->EnableInputWindow(false);
  visible_ = false;

  StartShowHideTimeline();

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_HIDDEN);
}

void DashController::StartShowHideTimeline()
{
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
      self->window_->ShowWindow(false, false);
    }

    return FALSE;
  }
  return TRUE;


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
