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


DECLARE_LOGGER(logger, "unity.appmanager.desktop.bamf");

namespace unity
{
namespace bamf
{
namespace
{
const char* UNSEEN_QUARK = "unity-unseen";
}


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

std::string View::type() const
{
  return glib::gchar_to_string(bamf_view_get_view_type(bamf_view_));
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


std::string WindowBase::type() const
{
  return View::type();
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
}

bool WindowBase::Focus() const
{
  Window xid = window_id();
  if (xid)
  {
    std::vector<Window> windows = { xid };
    // TODO: we should simplify the use case of focusing one window.
    // Somewhat outside the scope of these changes however.
    WindowManager::Default().FocusWindowGroup(
      windows,
      WindowManager::FocusVisibility::ForceUnminimizeInvisible,
      monitor(),true);
    return true;
  }
  return false;
}


AppWindow::AppWindow(ApplicationManager const& manager, glib::Object<BamfWindow> const& window)
  : WindowBase(manager, glib::object_cast<BamfView>(window))
  , bamf_window_(window)
{}

AppWindow::AppWindow(ApplicationManager const& manager, glib::Object<BamfView> const& window)
  : WindowBase(manager, window)
  , bamf_window_(glib::object_cast<BamfWindow>(window))
{}

Window AppWindow::window_id() const
{
  return bamf_window_get_xid(bamf_window_);
}

int AppWindow::monitor() const
{
  return bamf_window_get_monitor(bamf_window_);
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

Tab::Tab(ApplicationManager const& manager, glib::Object<BamfView> const& tab)
  : WindowBase(manager, tab)
  , bamf_tab_(glib::object_cast<BamfTab>(tab))
{}

Tab::Tab(ApplicationManager const& manager, glib::Object<BamfTab> const& tab)
  : WindowBase(manager, glib::object_cast<BamfView>(tab))
  , bamf_tab_(tab)
{}

Window Tab::window_id() const
{
  return bamf_tab_get_xid(bamf_tab_);
}

int Tab::monitor() const
{
  // TODO, we could find the real window for the window_id, and get the monitor for that.
  return -1;
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

// Being brutal with this function.
ApplicationWindowPtr create_window(ApplicationManager const& manager, glib::Object<BamfView> const& view)
{
  if (view.IsType(BAMF_TYPE_WINDOW))
  {
    return std::make_shared<AppWindow>(manager, view);
  }
  else if (view.IsType(BAMF_TYPE_TAB))
  {
    return std::make_shared<Tab>(manager, view);
  }
  // We don't handle applications here.
  return nullptr;
}

Application::Application(ApplicationManager const& manager, glib::Object<BamfView> const& app)
  : View(manager, app)
  , bamf_app_(glib::object_cast<BamfApplication>(app))
{
  HookUpEvents();
}

Application::Application(ApplicationManager const& manager, glib::Object<BamfApplication> const& app)
  : View(manager, glib::object_cast<BamfView>(app))
  , bamf_app_(app)
{
  HookUpEvents();
}

void Application::HookUpEvents()
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
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "running-changed",
  [this] (BamfView*, gboolean running) {
    LOG_TRACE(logger) << "running " << visible;
    this->running.changed.emit(running);
  });
  signals_.Add<void, BamfView*, gboolean>(bamf_view_, "urgent-changed",
  [this] (BamfView*, gboolean urgent) {
    this->urgent.changed.emit(urgent);
  });
  signals_.Add<void, BamfView*>(bamf_view_, "closed",
  [this] (BamfView*) {
    this->closed.emit();
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-added",
  [this] (BamfView*, BamfView* child) {
    // Ownership is not passed on signals
    glib::Object<BamfView> view(child, glib::AddRef());
    ApplicationWindowPtr const& win = create_window(this->manager_, view);
    if (win)
      this->window_opened.emit(*win);
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-removed",
  [this] (BamfView*, BamfView* child) {
    this->window_closed.emit();
  });

  signals_.Add<void, BamfView*, BamfView*>(bamf_view_, "child-moved",
  [this] (BamfView*, BamfView* child) {
    // Ownership is not passed on signals
    glib::Object<BamfView> view(child, glib::AddRef());
    ApplicationWindowPtr const& win = create_window(this->manager_, view);
    if (win)
      this->window_moved.emit(*win);
  });
}

std::string Application::GetDesktopFile() const
{
  return glib::gchar_to_string(bamf_application_get_desktop_file(bamf_app_));
}

std::string Application::type() const
{
  // Can't determine the type of a non-running app.
  std::string result = "unknown";
  if (running())
  {
    const gchar* type = bamf_application_get_application_type(bamf_app_);
    if (type) result = type;
  }
  return result;
}

std::string Application::repr() const
{
  std::ostringstream sout;
  sout << "<bamf::Application " << bamf_app_.RawPtr() << " >";
  return sout.str();
}

WindowList Application::GetWindows() const
{
  WindowList result;

  if (!bamf_app_)
    return result;

  std::shared_ptr<GList> children(bamf_view_get_children(bamf_view_), g_list_free);
  for (GList* l = children.get(); l; l = l->next)
  {
    glib::Object<BamfView> view(BAMF_VIEW(l->data), glib::AddRef());
    ApplicationWindowPtr const& window(create_window(manager_, view));
    if (window)
      result.push_back(window);
  }
  return result;
}

bool Application::OwnsWindow(Window window_id) const
{
  if (!window_id)
    return false;

  bool owns = false;
  std::shared_ptr<GList> children(bamf_view_get_children(bamf_view_), g_list_free);
  for (GList* l = children.get(); l && !owns; l = l->next)
  {
    owns = BAMF_IS_WINDOW(l->data) &&
           bamf_window_get_xid(static_cast<BamfWindow*>(l->data)) == window_id;
  }

  return owns;
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
  glib::Object<BamfView> view(bamf_application_get_focusable_child(bamf_app_),
                              glib::AddRef());
  return create_window(manager_, view);
}

void Application::Focus(bool show_only_visible, int monitor) const
{
  WindowManager& wm = WindowManager::Default();
  std::vector<Window> urgent_windows;
  std::vector<Window> visible_windows;
  std::vector<Window> non_visible_windows;
  bool any_visible = false;

  for (auto& window : GetWindows())
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

void Application::Quit() const
{
  for (auto& window : GetWindows())
  {
    window->Quit();
  }
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

bool Application::SetSeen(bool const& param)
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

bool Application::SetSticky(bool const& param)
{
  bool is_sticky = GetSticky();
  if (param == is_sticky)
    return false; // unchanged

  bamf_view_set_sticky(bamf_view_, param);
  return true; // value updated
}


Manager::Manager()
 : matcher_(bamf_matcher_get_default())
{
  LOG_TRACE(logger) << "Create BAMF Application Manager";
  signals_.Add<void, BamfMatcher*, BamfView*> (matcher_, "view-opened",
    sigc::mem_fun(this, &Manager::OnViewOpened));

  signals_.Add<void, BamfMatcher*, BamfView*, BamfView*>(matcher_, "active-window-changed",
  [this](BamfMatcher*, BamfView* /* from */, BamfView* to) {
    // Ownership is not passed on signals
    glib::Object<BamfView> view(to, glib::AddRef());
    ApplicationWindowPtr const& win = create_window(*this, view);
    if (win)
      this->active_window_changed.emit(win);
  });

  signals_.Add<void, BamfMatcher*, BamfApplication*, BamfApplication*> (matcher_, "active-application-changed",
  [this](BamfMatcher*, BamfApplication* /* from */, BamfApplication* to) {
    if (to)
    {
      // Ownership is not passed on signals
      glib::Object<BamfApplication> bamf_app(to, glib::AddRef());
      auto app = std::make_shared<Application>(*this, bamf_app);
      this->active_application_changed.emit(app);
    }
    else
    {
      this->active_application_changed.emit(nullptr);
    }
  });
}

Manager::~Manager()
{
  LOG_TRACE(logger) << "Manager::~Manager";
}

ApplicationWindowPtr Manager::GetActiveWindow()
{
  // No transfer of ownership for bamf_matcher_get_active_window.
  BamfWindow* active_win = bamf_matcher_get_active_window(matcher_);

  if (!active_win)
    return nullptr;

  // If the active window is a dock type, then we want the first visible, non-dock type.
  if (bamf_window_get_window_type(active_win) == BAMF_WINDOW_DOCK)
  {
    LOG_DEBUG(logger) << "Is a dock, looking at the window stack.";

    std::shared_ptr<GList> windows(bamf_matcher_get_window_stack_for_monitor(matcher_, -1), g_list_free);
    WindowManager& wm = WindowManager::Default();
    active_win = nullptr;

    for (GList *l = windows.get(); l; l = l->next)
    {
      if (!BAMF_IS_WINDOW(l->data))
      {
        LOG_DEBUG(logger) << "Window stack returned something not a window, WTF?";
        continue;
      }

      auto win = static_cast<BamfWindow*>(l->data);
      auto view = static_cast<BamfView*>(l->data);
      auto xid = bamf_window_get_xid(win);

      if (bamf_view_is_user_visible(view) &&
          bamf_window_get_window_type(win) != BAMF_WINDOW_DOCK &&
          wm.IsWindowOnCurrentDesktop(xid) &&
          wm.IsWindowVisible(xid))
      {
        active_win = win;
      }
    }
  }

  if (active_win)
  {
    glib::Object<BamfWindow> win(active_win, glib::AddRef());
    return std::make_shared<AppWindow>(*this, win);
  }

  return nullptr;
}

ApplicationPtr Manager::GetApplicationForDesktopFile(std::string const& desktop_file)
{
  glib::Object<BamfApplication> app(bamf_matcher_get_application_for_desktop_file(
    matcher_, desktop_file.c_str(), TRUE), glib::AddRef());

  if (app)
    return std::make_shared<Application>(*this, app);

  return nullptr;
}

ApplicationPtr Manager::GetApplicationForWindow(Window xid)
{
  glib::Object<BamfApplication> app(bamf_matcher_get_application_for_xid(matcher_, xid),
                                    glib::AddRef());
  if (app)
    return std::make_shared<Application>(*this, app);

  return nullptr;
}

ApplicationList Manager::GetRunningApplications()
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

    glib::Object<BamfApplication> bamf_app(static_cast<BamfApplication*>(l->data), glib::AddRef());
    auto app = std::make_shared<Application>(*this, bamf_app);
    result.push_back(app);
    LOG_DEBUG(logger) << "Running app: " << app->title();
  }
  return result;
}


void Manager::OnViewOpened(BamfMatcher* matcher, BamfView* view)
{
  LOG_TRACE_BLOCK(logger);
  if (!BAMF_IS_APPLICATION(view))
  {
    LOG_DEBUG(logger) << "view is not an app";
    return;
  }

  glib::Object<BamfView> app(view, glib::AddRef());
  application_started.emit(std::make_shared<Application>(*this, app));
}

} // namespace bamf
} // namespace unity
