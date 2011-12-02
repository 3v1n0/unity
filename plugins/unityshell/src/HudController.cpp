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
 * Authored by: Gord Allott <gord.allott@canonical.com>
 */

#include "HudController.h"

#include <NuxCore/Logger.h>
#include <Nux/HLayout.h>
#include "PluginAdapter.h"
#include "UBusMessages.h"
#include "UScreen.h"
namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.controller");
}

Controller::Controller()
  : launcher_width(66)
  , panel_height(24)
  , hud_service_("com.canonical.hud", "/com/canonical/hud")
  , window_(0)
  , visible_(false)
  , need_show_(false)
  , timeline_id_(0)
  , last_opacity_(0.0f)
  , start_time_(0)
{
  LOG_DEBUG(logger) << "hud startup";
  SetupRelayoutCallbacks();

  ubus.RegisterInterest(UBUS_HUD_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  //!!FIXME!! - just hijacks the dash close request so we get some more requests than normal,
  ubus.RegisterInterest(UBUS_PLACE_VIEW_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  PluginAdapter::Default()->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));

  hud_service_.suggestion_search_finished.connect(sigc::mem_fun(this, &Controller::OnSuggestionsFinished));
  EnsureHud();
}

Controller::~Controller()
{
  if (window_)
    window_->UnReference();
  window_ = 0;

  g_source_remove(timeline_id_);
  g_source_remove(ensure_id_);
}

void Controller::SetupWindow()
{
  window_ = new nux::BaseWindow("Hud");
  window_->SinkReference();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&Controller::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));
}

void Controller::SetupHudView()
{
  view_ = new View();

  nux::HLayout* layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  layout->AddView(view_, 1);
  layout->SetContentDistribution(nux::eStackLeft);
  layout->SetVerticalExternalMargin(0);
  layout->SetHorizontalExternalMargin(0);
  layout->SetMaximumWidth(940);
  layout->SetMaximumHeight(240);
  window_->SetLayout(layout);

  view_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  LOG_DEBUG(logger) << "connecting to signals";
  view_->search_changed.connect(sigc::mem_fun(this, &Controller::OnSearchChanged));
  view_->search_activated.connect(sigc::mem_fun(this, &Controller::OnSearchActivated));
  hud_service_.target_icon.changed.connect([&] (std::string icon_name) {
    view_->SetIcon(icon_name);
  });
}

void Controller::SetupRelayoutCallbacks()
{
  GdkScreen* screen = gdk_screen_get_default();

  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "monitors-changed", sigc::mem_fun(this, &Controller::Relayout)));
  sig_manager_.Add(new glib::Signal<void, GdkScreen*>(screen,
    "size-changed", sigc::mem_fun(this, &Controller::Relayout)));
}

void Controller::EnsureHud()
{
  if (window_)
    return;

  LOG_DEBUG(logger) << "Initializing Hud";

  SetupWindow();
  SetupHudView();
  Relayout();
  ensure_id_ = 0;
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

nux::Geometry Controller::GetIdealWindowGeometry()
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

void Controller::Relayout(GdkScreen*screen)
{
  EnsureHud();

  nux::Geometry geo = GetIdealWindowGeometry();
  window_->SetGeometry(geo);
  view_->Relayout();
}

void Controller::OnMouseDownOutsideWindow(int x, int y,
                                              unsigned long bflags, unsigned long kflags)
{
  LOG_DEBUG(logger) << "OnMouseDownOutsideWindow called";
  HideHud();
}

void Controller::OnScreenUngrabbed()
{
  LOG_DEBUG(logger) << "OnScreenUngrabbed called";
  if (need_show_)
  {
    EnsureHud();
    ShowHud();
  }
}

void Controller::OnExternalShowHud(GVariant* variant)
{
  EnsureHud();
  visible_ ? HideHud() : ShowHud();
}

void Controller::OnExternalHideHud(GVariant* variant)
{
  LOG_DEBUG(logger) << "External Hiding the hud";
  EnsureHud();
  HideHud();
}

void Controller::ShowHud()
{
  LOG_DEBUG(logger) << "Showing the hud";
  EnsureHud();

  window_->ShowWindow(true);
  window_->PushToFront();
  window_->EnableInputWindow(true, "Hud", true, false);
  window_->SetInputFocus();
  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
  window_->CaptureMouseDownAnyWhereElse(true);
  view_->CaptureMouseDownAnyWhereElse(true);
  window_->QueueDraw();
  window_->SetOpacity(1.0f);

  view_->ResetToDefault();
  hud_service_.GetSuggestions("");

  need_show_ = false;
  visible_ = true;
}
void Controller::HideHud(bool restore)
{
  LOG_DEBUG (logger) << "hiding the hud";
  //~ if (!visible_)
   //~ return;

  EnsureHud();

  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Hud", true, false);
  visible_ = false;
  window_->SetOpacity(0.0f);
  window_->ShowWindow(false);

  restore = true;
  if (restore)
    PluginAdapter::Default ()->restoreInputFocus ();
}

void Controller::StartShowHideTimeline()
{
  EnsureHud();

  if (timeline_id_)
    g_source_remove(timeline_id_);

  timeline_id_ = g_timeout_add(15, (GSourceFunc)Controller::OnViewShowHideFrame, this);
  last_opacity_ = window_->GetOpacity();
  start_time_ = g_get_monotonic_time();

}

gboolean Controller::OnViewShowHideFrame(Controller* self)
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

void Controller::OnActivateRequest(GVariant* variant)
{
  EnsureHud();
  ShowHud();
}

void Controller::OnSearchChanged(std::string search_string)
{
  LOG_DEBUG(logger) << "Search Changed";
  hud_service_.GetSuggestions(search_string);
}

void Controller::OnSearchActivated(std::string search_string)
{
  hud_service_.Execute(search_string);
  HideHud();
}


void Controller::OnSuggestionsFinished(Hud::Suggestions suggestions)
{
  view_->SetSuggestions(suggestions);
}

// Introspectable
const gchar* Controller::GetName()
{
  return "unity.hud.Controller";
}

void Controller::AddProperties(GVariantBuilder* builder)
{}


}
}
