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

#include "unity-shared/ApplicationManager.h"



using namespace unity;

int main()
{
  ApplicationManager& manager = ApplicationManager::Default();

  ApplicationPtr active_app = manager.active_application();

  if (active_app)
  {
    std::cout << "Active app: " << active_app->title() << "\n";
  }
  else
  {
    std::cout << "No Active app\n";
  }
}