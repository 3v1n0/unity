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
#include "PanelStyle.h"
#include "UBusMessages.h"
#include "UScreen.h"

#include <libbamf/libbamf.h>

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

  ubus.RegisterInterest(UBUS_OVERLAY_SHOWN, [&] (GVariant *data) {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, &overlay_identity, &can_maximise, &overlay_monitor);

    if (g_strcmp0(overlay_identity, "hud"))
    {
      HideHud(true);
    }
  });

  PluginAdapter::Default()->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));

  hud_service_.queries_updated.connect(sigc::mem_fun(this, &Controller::OnQueriesFinished));
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
  LOG_DEBUG(logger) << "SetupHudView called";
  view_ = new View();

  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(view_, 1, nux::MINOR_POSITION_TOP);
  window_->SetLayout(layout_);

  view_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  LOG_DEBUG(logger) << "connecting to signals";
  view_->search_changed.connect(sigc::mem_fun(this, &Controller::OnSearchChanged));
  view_->search_activated.connect(sigc::mem_fun(this, &Controller::OnSearchActivated));
  view_->query_activated.connect(sigc::mem_fun(this, &Controller::OnQueryActivated));
  view_->query_selected.connect(sigc::mem_fun(this, &Controller::OnQuerySelected));
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
   int primary_monitor = uscreen->GetMonitorWithMouse();
   auto monitor_geo = uscreen->GetMonitorGeometry(primary_monitor);

   // We want to cover as much of the screen as possible to grab any mouse events outside
   // of our window
   panel::Style &panel_style = panel::Style::Instance();
   return nux::Geometry (monitor_geo.x,
                         monitor_geo.y + panel_style.panel_height,
                         monitor_geo.width,
                         monitor_geo.height - panel_style.panel_height);
}

void Controller::Relayout(GdkScreen*screen)
{
  EnsureHud();
  nux::Geometry content_geo = view_->GetGeometry();
  nux::Geometry geo = GetIdealWindowGeometry();

  window_->SetGeometry(geo);
  layout_->SetMinMaxSize(content_geo.width, content_geo.height);
  view_->SetWindowGeometry(window_->GetAbsoluteGeometry(), window_->GetGeometry());
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

void Controller::ShowHideHud()
{
  EnsureHud();
  visible_ ? HideHud(true) : ShowHud();
}

bool Controller::IsVisible()
{
  return visible_;
}

void Controller::ShowHud()
{
  PluginAdapter* adaptor = PluginAdapter::Default();
  LOG_DEBUG(logger) << "Showing the hud";
  EnsureHud();
  
  if (visible_ || adaptor->IsExpoActive() || adaptor->IsScaleActive())
   return;
  
  view_->AboutToShow();

  // we first want to grab the currently active window, luckly we can just ask the jason interface(bamf)
  BamfMatcher* matcher = bamf_matcher_get_default();
  glib::Object<BamfView> bamf_app((BamfView*)(bamf_matcher_get_active_application(matcher)));
  focused_app_icon_ = bamf_view_get_icon(bamf_app);

  LOG_DEBUG(logger) << "Taking application icon: " << focused_app_icon_;
  view_->SetIcon(focused_app_icon_);

  window_->ShowWindow(true);
  window_->PushToFront();
  window_->EnableInputWindow(true, "Hud", true, false);
  window_->SetInputFocus();
  window_->CaptureMouseDownAnyWhereElse(true);
  view_->CaptureMouseDownAnyWhereElse(true);
  window_->QueueDraw();

  view_->ResetToDefault();
  need_show_ = false;
  visible_ = true;

  StartShowHideTimeline();
  view_->SetWindowGeometry(window_->GetAbsoluteGeometry(), window_->GetGeometry());

  // hide the launcher
  GVariant* message_data = g_variant_new("(b)", TRUE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, UScreen::GetDefault()->GetMonitorWithMouse());
  ubus.SendMessage(UBUS_OVERLAY_SHOWN, info);
  
  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
}
void Controller::HideHud(bool restore)
{
  LOG_DEBUG (logger) << "hiding the hud";
  if (visible_ == false)
    return;

  EnsureHud();
  view_->AboutToHide();
  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Hud", true, false);
  visible_ = false;

  StartShowHideTimeline();

  restore = true;
  if (restore)
    PluginAdapter::Default ()->restoreInputFocus ();

  hud_service_.CloseQuery();

  //unhide the launcher
  GVariant* message_data = g_variant_new("(b)", FALSE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, 0);
  ubus.SendMessage(UBUS_OVERLAY_HIDDEN, info);
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
    else
    {
      // ensure the text entry is focused
      nux::GetWindowCompositor().SetKeyFocusArea(self->view_->default_focus());
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
  hud_service_.RequestQuery(search_string);
}

void Controller::OnSearchActivated(std::string search_string)
{
  unsigned int timestamp = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent().x11_timestamp;
  hud_service_.ExecuteQueryBySearch(search_string, timestamp);
  HideHud();
}

void Controller::OnQueryActivated(Query::Ptr query)
{
  LOG_DEBUG(logger) << "Activating query, " << query->formatted_text;
  unsigned int timestamp = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent().x11_timestamp;
  hud_service_.ExecuteQuery(query, timestamp);
  HideHud();
}

void Controller::OnQuerySelected(Query::Ptr query)
{
  LOG_DEBUG(logger) << "Selected query, " << query->formatted_text;
  view_->SetIcon(query->icon_name);
}


void Controller::OnQueriesFinished(Hud::Queries queries)
{
  view_->SetQueries(queries);
  std::string icon_name = focused_app_icon_;
  for (auto query = queries.begin(); query != queries.end(); query++)
  {
    if (!(*query)->icon_name.empty())
    {
      icon_name = (*query)->icon_name;
      break;
    }
  }

  LOG_DEBUG(logger) << "setting icon to - " << icon_name;
  view_->SetIcon(icon_name);
}

// Introspectable
std::string Controller::GetName() const
{
  return "HudController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  g_variant_builder_add(builder, "{sv}", "visible", g_variant_new_boolean (visible_));
}


}
}
