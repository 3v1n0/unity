// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Tim Penhey <tim.penhey@canonical.com>
 */

#include "unity-shared/BamfApplicationManager.h"
#include "unity-shared/WindowManager.h"

#include <NuxCore/Logger.h>
#include <NuxGraphics/XInputWindow.h>
#include <UnityCore/DesktopUtilities.h>


DECLARE_LOGGER(logger, "unity.appmanager.desktop.bamf");

namespace unity
{
namespace bamf
{
namespace
{
const char* UNSEEN_QUARK = "unity-unseen";

namespace pool
{
// We keep a cache on views here, it would be nice to clean these on BAMF reload
std::unordered_map<BamfView*, ApplicationPtr> apps_;
std::unordered_map<BamfView*, ApplicationWindowPtr> wins_;

ApplicationPtr EnsureApplication(ApplicationManager const& manager, BamfView* view)
{
  if (!BAMF_IS_APPLICATION(view))
    return nullptr;

  auto it = apps_.find(view);

  if (it != apps_.end())
    return it->second;

  glib::Object<BamfApplication> bamfapp(reinterpret_cast<BamfApplication*>(view), glib::AddRef());
  auto const& app = std::make_shared<Application>(manager, bamfapp);
  apps_.insert({view, app});
  return app;
}

ApplicationWindowPtr EnsureWindow(ApplicationManager const& manager, BamfView* view)
{
  if (!BAMF_IS_WINDOW(view))
    return nullptr;

  auto it = wins_.find(view);

  if (it != wins_.end())
    return it->second;

  glib::Object<BamfWindow> bamfwin(reinterpret_cast<BamfWindow*>(view), glib::AddRef());
  auto const& win = std::make_shared<AppWindow>(manager, bamfwin);
  wins_.insert({view, win});
  return win;
}

} // pool namespace
} // anonymous namespace


// Due to the way glib handles object inheritance, we need to cast between pointer types.
// In order to make the up-call for the base class easy, we pass through a void* for the view.
View::View(ApplicationManager const& manager, glib::Object<BamfView> const& view)
  : manager_(manager)
  , bamf_view_(view)
{}

std::string View::GetTitle() const
{
  return glib::String(bamf_view_get_name(bamf_view_)).Str();
}

std::string View::GetIcon() const
{
  return glib::String(bamf_view_get_icon(bamf_view_)).Str();
}

bool View::GetVisible() const
{
  return bamf_view_is_user_visible(bamf_view_);
}

bool View::GetActive() const
{
  return bamf_view_is_active(bamf_view_);
}

bool View::GetRunning() const
{
  return bamf_view_is_running(bamf_view_);
}

bool View::GetUrgent() const
{
  return bamf_view_is_urgent(bamf_view_);
}

bool View::GetStarting() const
{
  return bamf_view_is_starting(bamf_view_);
}


WindowBase::WindowBase(ApplicationManager const& manager,
                       glib::Object<BamfView> const& window)
  : View(manager, window)
{
  title.SetGetterFunction(std::bind(&View::GetTitle, this));
  icon.SetGetterFunction(std::bind(&View::GetIcon, this));
  visible.SetGetterFunction(std::bind(&View::GetVisible, this));
  active.SetGetterFunction(std::bind(&View::GetActive, this));
  urgent.SetGetterFunction(std::bind(&View::GetUrgent, this));

  signals_.Add<void, BamfView*, const char*, const char*>(bamf_view_, "name-changed",
  [this] (BamfView*, const char*, const char* new_name) {
    this->title.changed.emit(glib::gchar_to_string(new_name));
  });
  signals_.Add<void, BamfView*, const char*>(bamf_view_, "icon-changed",
  [this] (BamfView*, const char* icon) {
    this->icon.changed.emit(glib::gchar_to_string(icon));
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "user-visible-changed",
  [this] (BamfView*, gboolean visible) {
    this->visible.changed.emit(visible);
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "active-changed",
  [this] (BamfView*, gboolean active) {
    this->active.changed.emit(active);
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "urgent-changed",
  [this] (BamfView*, gboolean urgent) {
    this->urgent.changed.emit(urgent);
  });
  signals_.Add<void, BamfView*>(bamf_view_, "closed",
  [this] (BamfView* view) {
    this->closed.emit();
    pool::wins_.erase(view);
  });
}

bool WindowBase::Focus() const
{
  Window xid = window_id();
  if (xid)
  {
    auto& wm = WindowManager::Default();
    wm.UnMinimize(xid);
    wm.Raise(xid);
    wm.Activate(xid);
    return true;
  }
  return false;
}


AppWindow::AppWindow(ApplicationManager const& manager, glib::Object<BamfWindow> const& window)
  : AppWindow(manager, glib::object_cast<BamfView>(window))
{}

AppWindow::AppWindow(ApplicationManager const& manager, glib::Object<BamfView> const& window)
  : WindowBase(manager, window)
  , bamf_window_(glib::object_cast<BamfWindow>(window))
{
  monitor.SetGetterFunction(std::bind(&AppWindow::GetMonitor, this));
  maximized.SetGetterFunction(std::bind(&AppWindow::GetMaximized, this));

  signals_.Add<void, BamfWindow*, gint, gint>(bamf_window_, "monitor-changed",
  [this] (BamfWindow*, gint, gint monitor) {
    this->monitor.changed.emit(monitor);
  });
  signals_.Add<void, BamfWindow*, gint, gint>(bamf_window_, "maximized-changed",
  [this] (BamfWindow*, gint old_state, gint state) {
    if ((old_state == BAMF_WINDOW_MAXIMIZED) != (state == BAMF_WINDOW_MAXIMIZED))
      this->maximized.changed.emit(state == BAMF_WINDOW_MAXIMIZED);
  });
}

int AppWindow::GetMonitor() const
{
  return bamf_window_get_monitor(bamf_window_);
}

bool AppWindow::GetMaximized() const
{
  return bamf_window_maximized(bamf_window_) == BAMF_WINDOW_MAXIMIZED;
}

Window AppWindow::window_id() const
{
  return bamf_window_get_xid(bamf_window_);
}

WindowType AppWindow::type() const
{
  switch (bamf_window_get_window_type(bamf_window_))
  {
    case BAMF_WINDOW_NORMAL:
      return WindowType::NORMAL;
    case BAMF_WINDOW_DESKTOP:
      return WindowType::DESKTOP;
    case BAMF_WINDOW_DOCK:
      return WindowType::DOCK;
    case BAMF_WINDOW_DIALOG:
      return WindowType::DIALOG;
    case BAMF_WINDOW_TOOLBAR:
      return WindowType::TOOLBAR;
    case BAMF_WINDOW_MENU:
      return WindowType::MENU;
    case BAMF_WINDOW_UTILITY:
      return WindowType::UTILITY;
    case BAMF_WINDOW_SPLASHSCREEN:
      return WindowType::SPLASHSCREEN;
    default:
      return WindowType::UNKNOWN;
  }
}

ApplicationPtr AppWindow::application() const
{
  // Moderately evil, but better than changing the method to non-const.
  // We know that the manager will always be able to be non-const.
  ApplicationManager& m = const_cast<ApplicationManager&>(manager_);
  return m.GetApplicationForWindow(window_id());
}

void AppWindow::Quit() const
{
  WindowManager::Default().Close(window_id());
}

Tab::Tab(ApplicationManager const& manager, glib::Object<BamfTab> const& tab)
  : WindowBase(manager, glib::object_cast<BamfView>(tab))
  , bamf_tab_(tab)
{
  monitor.SetGetterFunction([] { return -1; });
}

Tab::Tab(ApplicationManager const& manager, glib::Object<BamfView> const& tab)
  : Tab(manager_, glib::object_cast<BamfTab>(tab))
{}

Window Tab::window_id() const
{
  return bamf_tab_get_xid(bamf_tab_);
}

WindowType Tab::type() const
{
  return WindowType::TAB;
}

ApplicationPtr Tab::application() const
{
  // TODO, we could find the real window for the window_id, and return the application for that.
  return ApplicationPtr();
}

bool Tab::Focus() const
{
  // Raise the tab in the browser.
  bamf_tab_raise(bamf_tab_);
  // Then raise the browser window.
  return WindowBase::Focus();
}

void Tab::Quit() const
{
  bamf_tab_close(bamf_tab_);
}

Application::Application(ApplicationManager const& manager, glib::Object<BamfView> const& app)
  : Application(manager, glib::object_cast<BamfApplication>(app))
{}

Application::Application(ApplicationManager const& manager, glib::Object<BamfApplication> const& app)
  : View(manager, glib::object_cast<BamfView>(app))
  , bamf_app_(app)
{
  // Hook up the property set/get functions
  using namespace std::placeholders;
  desktop_file.SetGetterFunction(std::bind(&Application::GetDesktopFile, this));
  title.SetGetterFunction(std::bind(&View::GetTitle, this));
  icon.SetGetterFunction(std::bind(&View::GetIcon, this));
  seen.SetGetterFunction(std::bind(&Application::GetSeen, this));
  seen.SetSetterFunction(std::bind(&Application::SetSeen, this, _1));
  sticky.SetGetterFunction(std::bind(&Application::GetSticky, this));
  sticky.SetSetterFunction(std::bind(&Application::SetSticky, this, _1));
  visible.SetGetterFunction(std::bind(&View::GetVisible, this));
  active.SetGetterFunction(std::bind(&View::GetActive, this));
  running.SetGetterFunction(std::bind(&View::GetRunning, this));
  urgent.SetGetterFunction(std::bind(&View::GetUrgent, this));
  starting.SetGetterFunction(std::bind(&View::GetStarting, this));


  signals_.Add<void, BamfApplication*, const char*>(bamf_app_, "desktop-file-updated",
  [this] (BamfApplication*, const char* new_desktop_file) {
    this->desktop_file.changed.emit(glib::gchar_to_string(new_desktop_file));
  });
  signals_.Add<void, BamfView*, const char*, const char*>(bamf_view_, "name-changed",
  [this] (BamfView*, const char*, const char* new_name) {
    this->title.changed.emit(glib::gchar_to_string(new_name));
  });
  signals_.Add<void, BamfView*, const char*>(bamf_view_, "icon-changed",
  [this] (BamfView*, const char* icon) {
    this->icon.changed.emit(glib::gchar_to_string(icon));
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "user-visible-changed",
  [this] (BamfView*, gboolean visible) {
    LOG_TRACE(logger) << "user-visible-changed " << visible;
    this->visible.changed.emit(visible);
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "active-changed",
  [this] (BamfView*, gboolean active) {
    LOG_TRACE(logger) << "active-changed " << visible;
    this->active.changed.emit(active);
  });

  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "starting-changed",
  [this] (BamfView*, gboolean starting) {
    LOG_TRACE(logger) << "starting " << starting;
    this->starting.changed.emit(starting);
  });

  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "running-changed",
  [this] (BamfView*, gboolean running) {
    LOG_TRACE(logger) << "running " << visible;
    UpdateWindows();
    this->running.changed.emit(running);
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "urgent-changed",
  [this] (BamfView*, gboolean urgent) {
    this->urgent.changed.emit(urgent);
  });
  signals_.Add<void, BamfView*>(bamf_view_, "closed",
  [this] (BamfView* view) {
    UpdateWindows();
    this->closed.emit();

    if (!sticky())
      pool::apps_.erase(view);
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-added",
  [this] (BamfView*, BamfView* child) {
    // Ownership is not passed on signals
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(manager_, child))
    {
      if (std::find(windows_.begin(), windows_.end(), win) == windows_.end())
      {
        windows_.push_back(win);
        this->window_opened.emit(win);
      }
    }
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-removed",
  [this] (BamfView*, BamfView* child) {
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(manager_, child))
    {
      windows_.erase(std::remove(windows_.begin(), windows_.end(), win), windows_.end());
      this->window_closed.emit(win);
    }
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-moved",
  [this] (BamfView*, BamfView* child) {
    // Ownership is not passed on signals
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(manager_, child))
      this->window_moved.emit(win);
  });

  UpdateWindows();
}

std::string Application::GetDesktopFile() const
{
  return glib::gchar_to_string(bamf_application_get_desktop_file(bamf_app_));
}

AppType Application::type() const
{
  // Can't determine the type of a non-running app.
  if (running())
  {
    auto type = glib::gchar_to_string(bamf_application_get_application_type(bamf_app_));

    if (type == "system")
      return AppType::NORMAL;

    if (type == "webapp")
      return AppType::WEBAPP;
  }

  return AppType::UNKNOWN;
}

std::string Application::repr() const
{
  std::ostringstream sout;
  sout << "<bamf::Application " << bamf_app_.RawPtr() << " >";
  return sout.str();
}

WindowList const& Application::GetWindows() const
{
  return windows_;
}

void Application::UpdateWindows()
{
  if (!bamf_app_ || !running() || bamf_view_is_closed(bamf_view_))
  {
    for (auto it = windows_.begin(); it != windows_.end();)
    {
      window_closed.emit(*it);
      it = windows_.erase(it);
    }

    return;
  }

  bool was_empty = windows_.empty();

  for (GList* l = bamf_view_peek_children(bamf_view_); l; l = l->next)
  {
    if (ApplicationWindowPtr const& window = pool::EnsureWindow(manager_, static_cast<BamfView*>(l->data)))
    {
      if (was_empty || std::find(windows_.begin(), windows_.end(), window) == windows_.end())
      {
        windows_.push_back(window);
        window_opened.emit(window);
      }
    }
  }
}

bool Application::OwnsWindow(Window window_id) const
{
  if (!window_id)
    return false;

  for (auto const& win : windows_)
  {
    if (win->window_id() == window_id)
      return true;
  }

  return false;
}

std::vector<std::string> Application::GetSupportedMimeTypes() const
{
  std::vector<std::string> result;
  std::unique_ptr<gchar*[], void(*)(gchar**)> mimes(
    bamf_application_get_supported_mime_types(bamf_app_), g_strfreev);

  if (mimes)
  {
    for (int i = 0; mimes[i]; i++)
    {
      result.push_back(mimes[i]);
    }
  }
  return result;
}

ApplicationWindowPtr Application::GetFocusableWindow() const
{
  return pool::EnsureWindow(manager_, bamf_application_get_focusable_child(bamf_app_));
}

void Manager::FocusWindowGroup(WindowList const& wins, bool show_only_visible, int monitor) const
{
  WindowManager& wm = WindowManager::Default();
  std::vector<Window> urgent_windows;
  std::vector<Window> visible_windows;
  std::vector<Window> non_visible_windows;
  bool any_visible = false;

  for (auto& window : wins)
  {
    Window window_id = window->window_id();
    if (window->urgent())
      urgent_windows.push_back(window_id);
    else if (window->visible())
      visible_windows.push_back(window_id);
    else
      non_visible_windows.push_back(window_id);

    if (wm.IsWindowOnCurrentDesktop(window_id) &&
        wm.IsWindowVisible(window_id))
    {
      any_visible = true;
    }
  }

  // This logic seems overly convoluted, but copying the behaviour from
  // the launcher icon for now.
  auto visibility = WindowManager::FocusVisibility::OnlyVisible;
  if (!show_only_visible)
  {
    visibility = any_visible
       ? WindowManager::FocusVisibility::ForceUnminimizeInvisible
       : WindowManager::FocusVisibility::ForceUnminimizeOnCurrentDesktop;
  }
  if (!urgent_windows.empty())
  {
    // Last param is whether to show only the top most window.  In the situation
    // where we have urgent windows, we want to raise all the urgent windows on
    // the current workspace, or the workspace of the top most urgent window.
    wm.FocusWindowGroup(urgent_windows, visibility, monitor, false);
  }
  else if (!visible_windows.empty())
  {
    wm.FocusWindowGroup(visible_windows, visibility, monitor, true);
  }
  else
  {
    // Not sure what the use case is for this behaviour, but at this stage,
    // copying behaviour from ApplicationLauncherIcon.
    wm.FocusWindowGroup(non_visible_windows, visibility, monitor, true);
  }
}

void Application::Focus(bool show_only_visible, int monitor) const
{
  manager_.FocusWindowGroup(GetWindows(), show_only_visible, monitor);
}

void Application::Quit() const
{
  for (auto& window : GetWindows())
    window->Quit();
}

bool Application::CreateLocalDesktopFile() const
{
  if (!desktop_file().empty())
    return false;

  glib::Object<BamfControl> control(bamf_control_get_default());
  bamf_control_create_local_desktop_file(control, bamf_app_);

  return true;
}

bool Application::GetSeen() const
{
  return g_object_get_qdata(glib::object_cast<GObject>(bamf_app_),
                            g_quark_from_string(UNSEEN_QUARK));
}

bool Application::SetSeen(bool param)
{
  bool is_seen = GetSeen();
  if (param == is_seen)
    return false; // unchanged

  void* data = param ? reinterpret_cast<void*>(1) : nullptr;
  g_object_set_qdata(glib::object_cast<GObject>(bamf_app_),
                     g_quark_from_string(UNSEEN_QUARK),
                     data);
  return true; // value updated
}

bool Application::GetSticky() const
{
  return bamf_view_is_sticky(bamf_view_);
}

bool Application::SetSticky(bool param)
{
  bool is_sticky = GetSticky();
  if (param == is_sticky)
    return false; // unchanged

  if (!param && bamf_view_is_closed(bamf_view_))
    pool::apps_.erase(bamf_view_);

  bamf_view_set_sticky(bamf_view_, param);
  return true; // value updated
}


Manager::Manager()
 : matcher_(bamf_matcher_get_default())
{
  LOG_TRACE(logger) << "Create BAMF Application Manager";
  signals_.Add<void, BamfMatcher*, BamfView*> (matcher_, "view-opened",
    sigc::mem_fun(this, &Manager::OnViewOpened));
  signals_.Add<void, BamfMatcher*, BamfView*> (matcher_, "view-closed",
    sigc::mem_fun(this, &Manager::OnViewClosed));

  signals_.Add<void, BamfMatcher*, BamfView*, BamfView*>(matcher_, "active-window-changed",
  [this](BamfMatcher*, BamfView* /* from */, BamfView* to) {
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(*this, to))
      this->active_window_changed.emit(win);
  });

  signals_.Add<void, BamfMatcher*, BamfApplication*, BamfApplication*> (matcher_, "active-application-changed",
  [this](BamfMatcher*, BamfApplication* /* from */, BamfApplication* to) {
    auto const& app = pool::EnsureApplication(*this, reinterpret_cast<BamfView*>(to));
    this->active_application_changed.emit(app);
  });
}

Manager::~Manager()
{
  LOG_TRACE(logger) << "Manager::~Manager";
}

ApplicationPtr Manager::GetUnityApplication() const
{
  auto const& our_xids = nux::XInputWindow::NativeHandleList();

  for (auto xid : our_xids)
  {
    auto *app_ptr = bamf_matcher_get_application_for_xid(matcher_, xid);

    if (ApplicationPtr const& app = pool::EnsureApplication(*this, reinterpret_cast<BamfView*>(app_ptr)))
      return app;
  }

  return GetApplicationForDesktopFile(DesktopUtilities::GetDesktopPathById("compiz.desktop"));
}

ApplicationPtr Manager::GetActiveApplication() const
{
  auto *app_ptr = bamf_matcher_get_active_application(matcher_);
  return pool::EnsureApplication(*this, reinterpret_cast<BamfView*>(app_ptr));
}

ApplicationWindowPtr Manager::GetActiveWindow() const
{
  if (BamfWindow* active_win = bamf_matcher_get_active_window(matcher_))
  {
    if (bamf_window_get_window_type(active_win) != BAMF_WINDOW_DOCK)
      return pool::EnsureWindow(*this, reinterpret_cast<BamfView*>(active_win));
  }

  // If the active window is a dock type, then we want the first visible, non-dock type.
  LOG_DEBUG(logger) << "Is a dock, looking at the window stack.";

  auto const& wins = GetWindowsForMonitor();
  WindowManager& wm = WindowManager::Default();

  for (auto it = wins.rbegin(); it != wins.rend(); ++it)
  {
    auto const& win = *it;
    auto xid = win->window_id();

    if (win->visible() &&
        win->type() != WindowType::DOCK &&
        wm.IsWindowOnCurrentDesktop(xid) &&
        wm.IsWindowVisible(xid))
    {
      return win;
    }
  }

  return nullptr;
}

ApplicationPtr Manager::GetApplicationForDesktopFile(std::string const& desktop_file) const
{
  auto* app = bamf_matcher_get_application_for_desktop_file(matcher_, desktop_file.c_str(), TRUE);
  return pool::EnsureApplication(*this, reinterpret_cast<BamfView*>(app));
}

ApplicationPtr Manager::GetApplicationForWindow(Window xid) const
{
  auto* app = bamf_matcher_get_application_for_xid(matcher_, xid);
  return pool::EnsureApplication(*this, reinterpret_cast<BamfView*>(app));
}

ApplicationWindowPtr Manager::GetWindowForId(Window xid) const
{
  if (xid == 0)
    return nullptr;

  for (auto const& win_pair : pool::wins_)
  {
    if (win_pair.second->window_id() == xid)
      return win_pair.second;
  }

  if (BamfWindow* win = bamf_matcher_get_window_for_xid(matcher_, xid))
    return pool::EnsureWindow(*this, reinterpret_cast<BamfView*>(win));

  auto* app = bamf_matcher_get_application_for_xid(matcher_, xid);

  if (!app)
    return nullptr;

  for (GList* l = bamf_view_peek_children(reinterpret_cast<BamfView*>(app)); l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
      continue;

    auto win = static_cast<BamfWindow*>(l->data);

    if (bamf_window_get_xid(win) == xid)
      return pool::EnsureWindow(*this, static_cast<BamfView*>(l->data));
  }

  return nullptr;
}

ApplicationList Manager::GetRunningApplications() const
{
  ApplicationList result;
  std::shared_ptr<GList> apps(bamf_matcher_get_applications(matcher_), g_list_free);

  for (GList *l = apps.get(); l; l = l->next)
  {
    if (!BAMF_IS_APPLICATION(l->data))
    {
      LOG_INFO(logger) << "Running apps given something not an app.";
      continue;
    }

    result.push_back(pool::EnsureApplication(*this, static_cast<BamfView*>(l->data)));
  }
  return result;
}

WindowList Manager::GetWindowsForMonitor(int monitor) const
{
  WindowList wins;
  std::shared_ptr<GList> windows(bamf_matcher_get_window_stack_for_monitor(matcher_, monitor), g_list_free);

  for (GList *l = windows.get(); l; l = l->next)
  {
    if (!BAMF_IS_WINDOW(l->data))
    {
      LOG_DEBUG(logger) << "Window stack returned something not a window, WTF?";
      continue;
    }

    auto bamf_win = static_cast<BamfWindow*>(l->data);

    if (bamf_window_get_window_type(bamf_win) != BAMF_WINDOW_DOCK)
      wins.push_back(pool::EnsureWindow(*this, static_cast<BamfView*>(l->data)));
  }

  return wins;
}

void Manager::OnViewOpened(BamfMatcher* matcher, BamfView* view)
{
  LOG_TRACE_BLOCK(logger);
  if (BAMF_IS_APPLICATION(view))
  {
    if (ApplicationPtr const& app = pool::EnsureApplication(*this, view))
      application_started.emit(app);
  }
  else if (BAMF_IS_WINDOW(view))
  {
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(*this, view))
      window_opened.emit(win);
  }
}

void Manager::OnViewClosed(BamfMatcher* matcher, BamfView* view)
{
  LOG_TRACE_BLOCK(logger);
  if (BAMF_IS_APPLICATION(view))
  {
    if (ApplicationPtr const& app = pool::EnsureApplication(*this, view))
      application_stopped.emit(app);
  }
  else if (BAMF_IS_WINDOW(view))
  {
    if (ApplicationWindowPtr const& win = pool::EnsureWindow(*this, view))
      window_closed.emit(win);
  }

  /* No removal here, it's done inside views, as 'closed' signal arrives later */
}

} // namespace bamf
} // namespace unity
