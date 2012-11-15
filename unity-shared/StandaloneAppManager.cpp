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
#include <gtk/gtk.h>

#include "unity-shared/ApplicationManager.h"



using namespace unity;

int main(int argc, char* argv[])
{
  g_type_init();
  gtk_init(&argc, &argv);
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  ApplicationManager& manager = ApplicationManager::Default();

  for (auto app : manager.running_applications())
  {
    std::cout << "Application: " << app->title() << "\n";

    for (auto win : app->get_windows())
    {
      std::cout << "  Window: " << win->title() << "\n";
    }
  }
  std::cout << "And a second time...\n";
  for (auto app : manager.running_applications())
  {
    std::cout << "Application: " << app->title() << "\n";

    for (auto win : app->get_windows())
    {
      std::cout << "  Window: " << win->title() << "\n";
    }
  }

}