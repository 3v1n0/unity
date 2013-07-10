/*
 * Copyright 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Tim Penhey  <tim.penhey@canonical.com>
 */

#include "mock-application.h"


namespace unity
{
// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
std::shared_ptr<ApplicationManager> create_application_manager()
{
  return std::make_shared<testmocks::MockApplicationManager::Nice>();
}

}
