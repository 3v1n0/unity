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
#include <UnityCore/Variant.h>

#include "unity-shared/WindowManager.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"

#include "config.h"
#include <libbamf/libbamf.h>

namespace unity
{
namespace hud
{

namespace
{
nux::logging::Logger logger("unity.hud.controller");
}

Controller::Controller(std::function<AbstractView*(void)> const& function)
  : launcher_width(64)
  , launcher_locked_out(false)
  , multiple_launchers(true)
  , hud_service_("com.canonical.hud", "/com/canonical/hud")
  , visible_(false)
  , need_show_(false)
  , timeline_animator_(90)
  , view_(nullptr)
  , monitor_index_(0)
  , view_function_(function)
{
  LOG_DEBUG(logger) << "hud startup";
  SetupWindow();
  UScreen::GetDefault()->changed.connect([&] (int, std::vector<nux::Geometry>&) { Relayout(true); });

  ubus.RegisterInterest(UBUS_HUD_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  //!!FIXME!! - just hijacks the dash close request so we get some more requests than normal,
  ubus.RegisterInterest(UBUS_PLACE_VIEW_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  ubus.RegisterInterest(UBUS_OVERLAY_SHOWN, [&] (GVariant *data) {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, &overlay_identity, &can_maximise, &overlay_monitor);

    if (overlay_identity.Str() != "hud")
    {
      HideHud(true);
    }
  });

  launcher_width.changed.connect([&] (int new_width) { Relayout(); });

  auto wm = WindowManager::Default();
  wm->compiz_screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));
  wm->initiate_spread.connect(sigc::bind(sigc::mem_fun(this, &Controller::HideHud), true));

  hud_service_.queries_updated.connect(sigc::mem_fun(this, &Controller::OnQueriesFinished));
  timeline_animator_.animation_updated.connect(sigc::mem_fun(this, &Controller::OnViewShowHideFrame));

  EnsureHud();
}

void Controller::SetupWindow()
{
  // Since BaseWindow is a View it is initially unowned.  This means that the first
  // reference that is taken grabs ownership of the pointer.  Since the smart pointer
  // references it, it becomes the owner, so no need to adopt the pointer here.
  window_ = new ResizingBaseWindow("Hud", [this](nux::Geometry const& geo)
  {
    if (view_)
      return GetInputWindowGeometry();
    return geo;
  });
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&Controller::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(
    sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  /* FIXME - first time we load our windows there is a race that causes the
   * input window not to actually get input, this side steps that by causing
   * an input window show and hide before we really need it. */
  auto wm = WindowManager::Default();
  wm->saveInputFocus ();
  window_->EnableInputWindow(true, "Hud", true, false);
  window_->EnableInputWindow(false, "Hud", true, false);
  wm->restoreInputFocus ();
}

void Controller::SetupHudView()
{
  LOG_DEBUG(logger) << "SetupHudView called";
  view_ = view_function_();

  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(view_, 1, nux::MINOR_POSITION_TOP);
  window_->SetLayout(layout_);

  window_->UpdateInputWindowGeometry();

  view_->mouse_down_outside_pointer_grab_area.connect(sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  LOG_DEBUG(logger) << "connecting to signals";
  view_->search_changed.connect(sigc::mem_fun(this, &Controller::OnSearchChanged));
  view_->search_activated.connect(sigc::mem_fun(this, &Controller::OnSearchActivated));
  view_->query_activated.connect(sigc::mem_fun(this, &Controller::OnQueryActivated));
  view_->query_selected.connect(sigc::mem_fun(this, &Controller::OnQuerySelected));
  view_->layout_changed.connect(sigc::bind(sigc::mem_fun(this, &Controller::Relayout), nullptr));
  // Add to the debug introspection.
  AddChild(view_);
}

int Controller::GetIdealMonitor()
{
  int ideal_monitor;
  if (window_->IsVisible())
    ideal_monitor = monitor_index_;
  else
    ideal_monitor = UScreen::GetDefault()->GetMonitorWithMouse();
  return ideal_monitor;
}

bool Controller::IsLockedToLauncher(int monitor)
{
  if (launcher_locked_out)
  {
    int primary_monitor = UScreen::GetDefault()->GetPrimaryMonitor();

    if (multiple_launchers || (!multiple_launchers && primary_monitor == monitor))
    {
      return true;
    }
  }

  return false;
}

void Controller::EnsureHud()
{
  if (!window_)
  {
    LOG_DEBUG(logger) << "Initializing Hud Window";
    SetupWindow();
  }

  if (!view_)
  {
    LOG_DEBUG(logger) << "Initializing Hud View";
    SetupHudView();
    Relayout();
  }
}

void Controller::SetIcon(std::string const& icon_name)
{
  LOG_DEBUG(logger) << "setting icon to - " << icon_name;

  if (view_)
    view_->SetIcon(icon_name, tile_size, icon_size, launcher_width - tile_size);

  ubus.SendMessage(UBUS_HUD_ICON_CHANGED, g_variant_new_string(icon_name.c_str()));
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

nux::Geometry Controller::GetIdealWindowGeometry()
{
  int ideal_monitor = GetIdealMonitor();
  auto monitor_geo = UScreen::GetDefault()->GetMonitorGeometry(ideal_monitor);

  // We want to cover as much of the screen as possible to grab any mouse events
  // outside of our window
  panel::Style &panel_style = panel::Style::Instance();
  nux::Geometry geo(monitor_geo.x,
                    monitor_geo.y + panel_style.panel_height,
                    monitor_geo.width,
                    monitor_geo.height - panel_style.panel_height);

  if (IsLockedToLauncher(ideal_monitor))
  {
    geo.x += launcher_width;
    geo.width -= launcher_width;
  }

  return geo;
}

void Controller::Relayout(bool check_monitor)
{
  EnsureHud();

  if (check_monitor)
  {
    monitor_index_ = CLAMP(GetIdealMonitor(), 0, static_cast<int>(UScreen::GetDefault()->GetMonitors().size()-1));
  }
  nux::Geometry const& geo = GetIdealWindowGeometry();

  view_->Relayout();
  window_->SetGeometry(geo);
  panel::Style &panel_style = panel::Style::Instance();
  view_->SetMonitorOffset(launcher_width, panel_style.panel_height);
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
    nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());

    window_->PushToFront();
    window_->SetInputFocus();
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

  if (variant)
  {
    HideHud(g_variant_get_boolean(variant));
  }
  else
  {
    HideHud();
  }
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
  WindowManager* adaptor = WindowManager::Default();
  LOG_DEBUG(logger) << "Showing the hud";
  EnsureHud();

  if (visible_ || adaptor->IsExpoActive() || adaptor->IsScaleActive())
   return;

  if (adaptor->IsScreenGrabbed())
  {
    need_show_ = true;
    return;
  }

  unsigned int ideal_monitor = GetIdealMonitor();

  if (ideal_monitor != monitor_index_)
  {
    Relayout();
    monitor_index_ = ideal_monitor;
  }

  view_->ShowEmbeddedIcon(!IsLockedToLauncher(monitor_index_));
  view_->AboutToShow();

  // We first want to grab the currently active window
  glib::Object<BamfMatcher> matcher(bamf_matcher_get_default());
  BamfWindow* active_win = bamf_matcher_get_active_window(matcher);

  Window active_xid = bamf_window_get_xid(active_win);
  std::vector<Window> const& unity_xids = nux::XInputWindow::NativeHandleList();

  // If the active window is an unity window, we must get the top-most valid window
  if (std::find(unity_xids.begin(), unity_xids.end(), active_xid) != unity_xids.end())
  {
    // Windows list stack for all the monitors
    GList *windows = bamf_matcher_get_window_stack_for_monitor(matcher, -1);

    // Reset values, in case we can't find a window ie. empty current desktop
    active_xid = 0;
    active_win = nullptr;

    for (GList *l = windows; l; l = l->next)
    {
      if (!BAMF_IS_WINDOW(l->data))
        continue;

      auto win = static_cast<BamfWindow*>(l->data);
      auto view = static_cast<BamfView*>(l->data);
      Window xid = bamf_window_get_xid(win);

      if (bamf_view_user_visible(view) && bamf_window_get_window_type(win) != BAMF_WINDOW_DOCK &&
          WindowManager::Default()->IsWindowOnCurrentDesktop(xid) &&
          WindowManager::Default()->IsWindowVisible(xid) &&
          std::find(unity_xids.begin(), unity_xids.end(), xid) == unity_xids.end())
      {
        active_win = win;
        active_xid = xid;
      }
    }

    g_list_free(windows);
  }

  BamfApplication* active_app = bamf_matcher_get_application_for_window(matcher, active_win);

  if (BAMF_IS_VIEW(active_app))
  {
    auto active_view = reinterpret_cast<BamfView*>(active_app);
    glib::String view_icon(bamf_view_get_icon(active_view));
    focused_app_icon_ = view_icon.Str();
  }
  else
  {
    focused_app_icon_ = focused_app_icon_ = PKGDATADIR "/launcher_bfb.png";
  }

  LOG_DEBUG(logger) << "Taking application icon: " << focused_app_icon_;
  SetIcon(focused_app_icon_);

  window_->ShowWindow(true);
  window_->PushToFront();
  window_->EnableInputWindow(true, "Hud", true, false);
  window_->UpdateInputWindowGeometry();
  window_->SetInputFocus();
  window_->CaptureMouseDownAnyWhereElse(true);
  view_->CaptureMouseDownAnyWhereElse(true);
  window_->QueueDraw();

  view_->ResetToDefault();
  need_show_ = true;
  visible_ = true;

  StartShowHideTimeline();

  // hide the launcher
  GVariant* message_data = g_variant_new("(b)", TRUE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);
  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, monitor_index_);
  ubus.SendMessage(UBUS_OVERLAY_SHOWN, info);

  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
  window_->SetEnterFocusInputArea(view_->default_focus());
}

void Controller::HideHud(bool restore)
{
  LOG_DEBUG (logger) << "hiding the hud";
  if (visible_ == false)
    return;

  need_show_ = false;
  EnsureHud();
  view_->AboutToHide();
  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Hud", true, false);
  visible_ = false;

  nux::GetWindowCompositor().SetKeyFocusArea(NULL,nux::KEY_NAV_NONE);

  StartShowHideTimeline();

  restore = true;
  if (restore)
    WindowManager::Default ()->restoreInputFocus ();

  hud_service_.CloseQuery();

  //unhide the launcher
  GVariant* message_data = g_variant_new("(b)", FALSE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);

  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, monitor_index_);
  ubus.SendMessage(UBUS_OVERLAY_HIDDEN, info);
}

void Controller::StartShowHideTimeline()
{
  EnsureHud();

  double current_opacity = window_->GetOpacity();
  timeline_animator_.Stop();
  timeline_animator_.Start(visible_ ? current_opacity : 1.0f - current_opacity);
}

void Controller::OnViewShowHideFrame(double progress)
{
  window_->SetOpacity(visible_ ? progress : 1.0f - progress);

  if (progress == 1.0f)
  {
    if (!visible_)
    {
      window_->ShowWindow(false);
      view_->ResetToDefault();
    }
    else
    {
      // ensure the text entry is focused
      nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
    }
  }
}

void Controller::OnActivateRequest(GVariant* variant)
{
  EnsureHud();
  ShowHud();
}

void Controller::OnSearchChanged(std::string search_string)
{
  // we're using live_search_reached, so this is called 40ms after the text
  // is input in the search bar
  LOG_DEBUG(logger) << "Search Changed";

  last_search_ = search_string;
  hud_service_.RequestQuery(last_search_);
}

void Controller::OnSearchActivated(std::string search_string)
{
  unsigned int timestamp = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent().x11_timestamp;
  hud_service_.ExecuteQueryBySearch(search_string, timestamp);
  ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
}

void Controller::OnQueryActivated(Query::Ptr query)
{
  LOG_DEBUG(logger) << "Activating query, " << query->formatted_text;
  unsigned int timestamp = nux::GetWindowThread()->GetGraphicsDisplay().GetCurrentEvent().x11_timestamp;
  hud_service_.ExecuteQuery(query, timestamp);
  ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
}

void Controller::OnQuerySelected(Query::Ptr query)
{
  LOG_DEBUG(logger) << "Selected query, " << query->formatted_text;
  SetIcon(query->icon_name);
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

  SetIcon(icon_name);
  view_->SearchFinished();
}

// Introspectable
std::string Controller::GetName() const
{
  return "HudController";
}

void Controller::AddProperties(GVariantBuilder* builder)
{
  variant::BuilderWrapper(builder)
    .add(window_ ? window_->GetGeometry() : nux::Geometry())
    .add("ideal_monitor", GetIdealMonitor())
    .add("visible", visible_)
    .add("hud_monitor", monitor_index_)
    .add("locked_to_launcher", IsLockedToLauncher(monitor_index_));
}

nux::Geometry Controller::GetInputWindowGeometry()
{
  EnsureHud();
  nux::Geometry const& window_geo(window_->GetGeometry());
  nux::Geometry const& view_content_geo(view_->GetContentGeometry());
  return nux::Geometry(window_geo.x, window_geo.y, view_content_geo.width, view_content_geo.height);
}



}
}
