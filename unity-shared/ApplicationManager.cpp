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

#include "unity-shared/ApplicationManager.h"

#include "unity-shared/WindowManager.h"

namespace unity
{
// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
std::shared_ptr<ApplicationManager> create_application_manager();

ApplicationManager& ApplicationManager::Default()
{
  static std::shared_ptr<ApplicationManager> instance(create_application_manager());
  return *instance;
}


// This method is needed to create an unresolved external for the
// WindowManager::Default method.  This is because it is highly likely that
// the application manager implementations need the window manager for some
// things, and in fact the bamf one does.  In order to make the linker happy,
// we need to make sure that we have this here.
void dummy()
{
  WindowManager::Default();
}

} // namespace unity
