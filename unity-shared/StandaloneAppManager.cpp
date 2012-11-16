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

#include "unity-shared/ApplicationManager.h"


using namespace std;
using namespace unity;

DECLARE_LOGGER(logger, "unity.appmanager.test");

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
         << "\n  icon: \"" << app->icon() << "\""
         << endl;

    for (auto win : app->get_windows())
    {
      std::cout << "  Window: " << win->title() << "\n";
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
    LOG_INFO(logger) << "Already seen " << app->title() << ", skipping event connection.";
    return;
  }
  std::string app_name = app->title();
  app->visible.changed.connect([app_name](bool const& value) {
    cout << app_name << " visibility changed: " << (value ? "yes" : "no") << endl;
  });
  app->running.changed.connect([app_name](bool const& value) {
    cout << app_name << " running changed: " << (value ? "yes" : "no") << endl;
  });
  app->active.changed.connect([app_name](bool const& value) {
    cout << app_name << " active changed: " << (value ? "yes" : "no") << endl;
  });
  app->closed.connect([app_name]() {
    cout << app_name << " closed." << endl;
  });
  app->seen = true;
}

int main(int argc, char* argv[])
{
  g_type_init();
  gtk_init(&argc, &argv);
  nux::logging::configure_logging(::getenv("UNITY_APP_LOG_SEVERITY"));

  ApplicationManager& manager = ApplicationManager::Default();

  ApplicationList apps = manager.running_applications();

  for (auto app : apps)
  {
    dump_app(app);
    connect_events(app);
  }

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

  shared_ptr<GMainLoop> main_loop(g_main_loop_new(nullptr, FALSE),
                                  g_main_loop_unref);
  g_main_loop_run(main_loop.get());
  cout << "After main loop.\n";
}
