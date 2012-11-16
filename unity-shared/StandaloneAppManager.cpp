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

#include <NuxCore/Logger.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "unity-shared/ApplicationManager.h"



using namespace std;
using namespace unity;

void dump_app(ApplicationPtr const& app)
{
  if (app)
  {
    cout << "Application: " << app->title()
         << ", seen: " << app->seen()
         << ", sticky: " << app->sticky()
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

int main(int argc, char* argv[])
{
  g_type_init();
  gtk_init(&argc, &argv);
  nux::logging::configure_logging(::getenv("UNITY_APP_LOG_SEVERITY"));

  ApplicationManager& manager = ApplicationManager::Default();

  for (auto app : manager.running_applications())
  {
    dump_app(app);
  }
  std::cout << "And a second time...\n";
  for (auto app : manager.running_applications())
  {
    dump_app(app);
  }

  // Get some desktop files for checking
  ApplicationPtr pgadmin = manager.GetApplicationForDesktopFile(
    "/usr/share/applications/pgadmin3.desktop");
  dump_app(pgadmin);
  ApplicationPtr gedit = manager.GetApplicationForDesktopFile(
    "/usr/share/applications/gedit.desktop");
  dump_app(gedit);
  pgadmin->sticky = true;
  pgadmin->seen = true;
  dump_app(pgadmin);
  pgadmin->visible.changed.connect([pgadmin](bool const& value) {
    cout << pgadmin->title() << " visibility changed: " << value << endl;
  });
  // dump new apps
  manager.application_started.connect(&dump_app);

  shared_ptr<GMainLoop> main_loop(g_main_loop_new(nullptr, FALSE),
                                  g_main_loop_unref);
  g_main_loop_run(main_loop.get());
  cout << "After main loop.\n";
}
