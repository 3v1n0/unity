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

#include "unity-shared/AnimationUtils.h"
#include "unity-shared/ApplicationManager.h"
#include "unity-shared/WindowManager.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UScreen.h"

#include "config.h"


namespace unity
{
namespace hud
{
DECLARE_LOGGER(logger, "unity.hud.controller");

Controller::Controller(Controller::ViewCreator const& create_view,
                       Controller::WindowCreator const& create_window)
  : launcher_width(64)
  , launcher_locked_out(false)
  , multiple_launchers(true)
  , hud_service_("com.canonical.hud", "/com/canonical/hud")
  , visible_(false)
  , need_show_(false)
  , view_(nullptr)
  , monitor_index_(0)
  , create_view_(create_view)
  , create_window_(create_window)
  , timeline_animator_(90)
{
  LOG_DEBUG(logger) << "hud startup";

  // As a default, the create_view_ function should just create a view.
  if (create_view == nullptr)
  {
    create_view_ = []() {
      return new hud::View;
    };
  }

  // As a default. the create_window_ function should just create a base window.
  if (create_window_ == nullptr)
  {
    create_window_ = [this]() {
      return new ResizingBaseWindow("Hud",
                                    [this](nux::Geometry const& geo) {
                                      if (view_)
                                        return GetInputWindowGeometry();
                                      return geo;
                                    });
    };
  }

  SetupWindow();
  UScreen::GetDefault()->changed.connect([this] (int, std::vector<nux::Geometry>&) { Relayout(true); });

  ubus.RegisterInterest(UBUS_HUD_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  //!!FIXME!! - just hijacks the dash close request so we get some more requests than normal,
  ubus.RegisterInterest(UBUS_OVERLAY_CLOSE_REQUEST, sigc::mem_fun(this, &Controller::OnExternalHideHud));

  ubus.RegisterInterest(UBUS_OVERLAY_SHOWN, [this] (GVariant *data) {
    unity::glib::String overlay_identity;
    gboolean can_maximise = FALSE;
    gint32 overlay_monitor = 0;
    int width, height;
    g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

    if (overlay_identity.Str() != "hud")
    {
      HideHud();
    }
  });

  launcher_width.changed.connect([this] (int) { Relayout(); });

  WindowManager& wm = WindowManager::Default();
  wm.screen_ungrabbed.connect(sigc::mem_fun(this, &Controller::OnScreenUngrabbed));
  wm.initiate_spread.connect(sigc::mem_fun(this, &Controller::HideHud));

  hud_service_.queries_updated.connect(sigc::mem_fun(this, &Controller::OnQueriesFinished));
  timeline_animator_.updated.connect(sigc::mem_fun(this, &Controller::OnViewShowHideFrame));

  EnsureHud();
}

void Controller::SetupWindow()
{
  // Since BaseWindow is a View it is initially unowned.  This means that the first
  // reference that is taken grabs ownership of the pointer.  Since the smart pointer
  // references it, it becomes the owner, so no need to adopt the pointer here.
  window_ = create_window_();
  window_->SetBackgroundColor(nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  window_->SetConfigureNotifyCallback(&Controller::OnWindowConfigure, this);
  window_->ShowWindow(false);
  window_->SetOpacity(0.0f);
  window_->mouse_down_outside_pointer_grab_area.connect(
    sigc::mem_fun(this, &Controller::OnMouseDownOutsideWindow));

  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
    /* FIXME - first time we load our windows there is a race that causes the
     * input window not to actually get input, this side steps that by causing
     * an input window show and hide before we really need it. */
    WindowManager& wm = WindowManager::Default();
    wm.SaveInputFocus();
    window_->EnableInputWindow(true, "Hud", true, false);
    window_->EnableInputWindow(false, "Hud", true, false);
    wm.RestoreInputFocus();
  }
}

void Controller::SetupHudView()
{
  LOG_DEBUG(logger) << "SetupHudView called";
  view_ = create_view_();

  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->AddView(view_, 1, nux::MINOR_POSITION_START);
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

nux::ObjectPtr<AbstractView> Controller::HudView() const
{
  return nux::ObjectPtr<AbstractView>(view_);
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

  view_->QueueDraw();
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
  HideHud();
}

void Controller::ShowHideHud()
{
  EnsureHud();
  visible_ ? HideHud() : ShowHud();
}

void Controller::ReFocusKeyInput()
{
  if (visible_)
  {
    window_->PushToFront();
    window_->SetInputFocus();
  }
}

bool Controller::IsVisible()
{
  return visible_;
}

void Controller::ShowHud()
{
  WindowManager& wm = WindowManager::Default();
  LOG_DEBUG(logger) << "Showing the hud";
  EnsureHud();

  if (visible_ || wm.IsExpoActive() || wm.IsScaleActive())
    return;

  if (wm.IsScreenGrabbed())
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

  ApplicationManager& app_manager = ApplicationManager::Default();
  ApplicationPtr active_application;
  ApplicationWindowPtr active_window = app_manager.GetActiveWindow();
  if (active_window)
    active_application = active_window->application();

  if (active_application)
  {
    focused_app_icon_ = active_application->icon();
  }
  else
  {
    focused_app_icon_ = PKGDATADIR "/launcher_bfb.png";
  }

  LOG_DEBUG(logger) << "Taking application icon: " << focused_app_icon_;
  SetIcon(focused_app_icon_);
  FocusWindow();

  view_->ResetToDefault();
  need_show_ = true;
  visible_ = true;

  StartShowHideTimeline();

  // hide the launcher
  GVariant* message_data = g_variant_new("(b)", TRUE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);

  auto const& view_content_geometry = view_->GetContentGeometry();
  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, monitor_index_,
                                 view_content_geometry.width, view_content_geometry.height);
  ubus.SendMessage(UBUS_OVERLAY_SHOWN, info);

  nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
  window_->SetEnterFocusInputArea(view_->default_focus());
}

void Controller::FocusWindow()
{
  window_->ShowWindow(true);
  window_->PushToFront();

  if (nux::GetWindowThread()->IsEmbeddedWindow())
  {
    window_->EnableInputWindow(true, "Hud", true, false);
    window_->UpdateInputWindowGeometry();
  }

  window_->SetInputFocus();
  window_->QueueDraw();
}

void Controller::HideHud()
{
  LOG_DEBUG (logger) << "hiding the hud";
  if (!visible_)
    return;

  need_show_ = false;
  EnsureHud();
  view_->AboutToHide();
  window_->CaptureMouseDownAnyWhereElse(false);
  window_->EnableInputWindow(false, "Hud", true, false);
  visible_ = false;

  auto& wc = nux::GetWindowCompositor();
  auto *key_focus_area = wc.GetKeyFocusArea();
  if (key_focus_area && key_focus_area->IsChildOf(view_))
    wc.SetKeyFocusArea(nullptr, nux::KEY_NAV_NONE);

  WindowManager::Default().RestoreInputFocus();

  StartShowHideTimeline();
  hud_service_.CloseQuery();

  //unhide the launcher
  GVariant* message_data = g_variant_new("(b)", FALSE);
  ubus.SendMessage(UBUS_LAUNCHER_LOCK_HIDE, message_data);

  auto const& view_content_geometry = view_->GetContentGeometry();
  GVariant* info = g_variant_new(UBUS_OVERLAY_FORMAT_STRING, "hud", FALSE, monitor_index_,
                                 view_content_geometry.width, view_content_geometry.height);
  ubus.SendMessage(UBUS_OVERLAY_HIDDEN, info);
}

void Controller::StartShowHideTimeline()
{
  EnsureHud();
  animation::StartOrReverseIf(timeline_animator_, visible_);
}

void Controller::OnViewShowHideFrame(double opacity)
{
  window_->SetOpacity(opacity);

  if (opacity == 0.0f && !visible_)
  {
    window_->ShowWindow(false);
  }
  else if (opacity == 1.0f && visible_)
  {
    // ensure the text entry is focused
    nux::GetWindowCompositor().SetKeyFocusArea(view_->default_focus());
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
  unsigned int timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
  hud_service_.ExecuteQueryBySearch(search_string, timestamp);
  ubus.SendMessage(UBUS_HUD_CLOSE_REQUEST);
}

void Controller::OnQueryActivated(Query::Ptr query)
{
  LOG_DEBUG(logger) << "Activating query, " << query->formatted_text;
  unsigned int timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
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
