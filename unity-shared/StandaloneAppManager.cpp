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

#include <iostream>
#include <vector>

#include <NuxCore/Logger.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <signal.h>

#include "unity-shared/ApplicationManager.h"
#include "UnityCore/GLibSource.h"

using namespace std;
using namespace unity;

DECLARE_LOGGER(logger, "unity.appmanager.test");

GMainLoop *loop;

void dump_app(ApplicationPtr const& app, std::string const& prefix = "")
{
  if (app)
  {
    cout << prefix << "Application: " << app->title()
         << ", seen: " << (app->seen() ? "yes" : "no")
         << ", sticky: " << (app->sticky() ? "yes" : "no")
         << ", visible: " << (app->visible() ? "yes" : "no")
         << ", active: " << (app->active() ? "yes" : "no")
         << ", running: " << (app->running() ? "yes" : "no")
         << ", urgent: " << (app->urgent() ? "yes" : "no")
         << ", repr: " << app->repr()
         << "\n  icon: \"" << app->icon() << "\""
         << "\n  desktop file: \"" << app->desktop_file() << "\""
         << "\n  type: \"" << app->type() << "\""
         << endl;

    for (auto win : app->GetWindows())
    {
      std::cout << "  Window: " << win->title()
                << ", window_id: " << win->window_id()
                << ", monitor: " << win->monitor()
                << ", type: " << win->type()
                << endl;
    }
  }
  else
  {
    cout << "App ptr is null" << endl;
  }
}

void connect_events(ApplicationPtr const& app)
{
  if (app->seen())
  {
    cout << "Already seen " << app->title() << ", skipping event connection.\n";
    return;
  }
  std::string app_name = app->title();
  app->title.changed.connect([&app_name](std::string const& value) {
    cout << app_name << " changed name to: " << value << endl;
    app_name = value;
  });
  app->icon.changed.connect([&app_name](std::string const& value) {
    cout << app_name << " icon changed: " << value << endl;
  });
  app->desktop_file.changed.connect([&app_name](std::string const& value) {
    cout << app_name << " desktop file changed: " << value << endl;
  });
  app->visible.changed.connect([&app_name](bool value) {
    cout << app_name << " visibility changed: " << (value ? "yes" : "no") << endl;
  });
  app->running.changed.connect([&app_name](bool value) {
    cout << app_name << " running changed: " << (value ? "yes" : "no") << endl;
  });
  app->active.changed.connect([&app_name](bool value) {
    cout << app_name << " active changed: " << (value ? "yes" : "no") << endl;
  });
  app->urgent.changed.connect([&app_name](bool value) {
    cout << app_name << " urgent changed: " << (value ? "yes" : "no") << endl;
  });
  app->closed.connect([&app_name]() {
    cout << app_name << " closed." << endl;
  });
  app->window_opened.connect([&app_name](ApplicationWindow const& window) {
    cout << "** " << app_name << " window opened: " << window.title() << endl;
  });
  app->window_closed.connect([&app_name]() {
    cout << "** " << app_name << " window closed" << endl;
  });
  app->window_moved.connect([&app_name](ApplicationWindow const& window) {
    cout << "** " << app_name << " window moved: " << window.title() << endl;
  });
  app->seen = true;
}



nux::logging::Level glog_level_to_nux(GLogLevelFlags log_level)
{
  // For some weird reason, ERROR is more critical than CRITICAL in gnome.
  if (log_level & G_LOG_LEVEL_ERROR)
    return nux::logging::Critical;
  if (log_level & G_LOG_LEVEL_CRITICAL)
    return nux::logging::Error;
  if (log_level & G_LOG_LEVEL_WARNING)
    return nux::logging::Warning;
  if (log_level & G_LOG_LEVEL_MESSAGE ||
      log_level & G_LOG_LEVEL_INFO)
    return nux::logging::Info;
  // default to debug.
  return nux::logging::Debug;
}

void capture_g_log_calls(const gchar* log_domain,
                         GLogLevelFlags log_level,
                         const gchar* message,
                         gpointer user_data)
{
  // If the environment variable is set, we capture the backtrace.
  static bool glog_backtrace = ::getenv("UNITY_LOG_GLOG_BACKTRACE");
  // If nothing else, all log messages from unity should be identified as such
  std::string module("unity");
  if (log_domain)
  {
    module += std::string(".") + log_domain;
  }
  nux::logging::Logger logger(module);
  nux::logging::Level level = glog_level_to_nux(log_level);
  if (level >= logger.GetEffectiveLogLevel())
  {
    std::string backtrace;
    if (glog_backtrace && level >= nux::logging::Warning)
    {
      backtrace = "\n" + nux::logging::Backtrace();
    }
    nux::logging::LogStream(level, logger.module(), "<unknown>", 0).stream()
      << message << backtrace;
  }
}

void print_active_window(ApplicationManager& manager)
{
  ApplicationWindowPtr win = manager.GetActiveWindow();
  if (win)
  {
    ApplicationPtr app = win->application();
    if (app)
      cout << "\n\nActive App: " << app->title();
    else
      cout << "\n\nNo app for window:";
    cout << "\nActive Window: " << win->title() << endl;
  }
  else
    cout << "\n\nNo active window: " << endl;
}

void clean_exit(int sig)
{
  if (loop && g_main_loop_is_running(loop))
    g_main_loop_quit(loop);
}

namespace unity
{
// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
std::shared_ptr<ApplicationManager> create_application_manager();
}

int main(int argc, char* argv[])
{
  gtk_init(&argc, &argv);
  nux::logging::configure_logging(::getenv("UNITY_APP_LOG_SEVERITY"));
  g_log_set_default_handler(capture_g_log_calls, NULL);

  bool show_active = (argc > 1 && std::string(argv[1]) == "show-active");
  //std::shared_ptr<ApplicationManager> manager_ptr = create_application_manager();
  //ApplicationManager& manager = *manager_ptr;
  ApplicationManager& manager = ApplicationManager::Default();

  ApplicationPtr terminal = manager.GetApplicationForDesktopFile(
    "/usr/share/applications/gnome-terminal.desktop");
  terminal->sticky = true; // this is needed to get the notifications...
  dump_app(terminal);
  connect_events(terminal);

  ApplicationList apps = manager.GetRunningApplications();

  for (auto const& app : apps)
  {
    dump_app(app);
    connect_events(app);
  }

  dump_app(manager.GetApplicationForDesktopFile(
    "/usr/share/applications/gnome-terminal.desktop"));

  // Get some desktop files for checking
  ApplicationPtr pgadmin = manager.GetApplicationForDesktopFile(
    "/usr/share/applications/pgadmin3.desktop");
  dump_app(pgadmin);
  ApplicationPtr gedit = manager.GetApplicationForDesktopFile(
    "/usr/share/applications/gedit.desktop");
  dump_app(gedit);
  // dump new apps
  manager.application_started.connect([&apps](ApplicationPtr const& app) {
    apps.push_back(app);
    dump_app(app, "\nApp started: ");
    connect_events(app);
  });
  manager.active_application_changed.connect([](ApplicationPtr const& app) {
    if (app)
      cout << "Manager::active_application_changed: " << app->title() << endl;
    else
      cout << "Manager::active_application_changed to nothing\n";
  });
  manager.active_window_changed.connect([](ApplicationWindowPtr const& win) {
    cout << "Manager::active_window_changed: " << win->title() << endl;
  });

  shared_ptr<GMainLoop> main_loop(g_main_loop_new(nullptr, FALSE),
                                  g_main_loop_unref);
  loop = main_loop.get();
  signal(SIGINT, clean_exit);
  {
    glib::SourceManager source_manager;
    if (show_active)
      source_manager.AddTimeoutSeconds(5, [&manager]() {
        print_active_window(manager);
        return true;
      });
    g_main_loop_run(loop);
  }
  cout << "After main loop.\n";
}
