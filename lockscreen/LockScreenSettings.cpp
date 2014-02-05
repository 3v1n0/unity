// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
* Copyright (C) 2013 Canonical Ltd
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
* Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
*/

#include "LockScreenSettings.h"

#include <NuxCore/Logger.h>

namespace unity
{
namespace lockscreen
{

DECLARE_LOGGER(logger, "unity.lockscreen.settings");

namespace
{
Settings* settings_instance = nullptr;
}

Settings::Settings()
{
  if (settings_instance)
  {
    LOG_ERROR(logger) << "More than one lockscreen::Settings created.";
  }
  else
  {
  	lockscreen_type = Type::UNITY;

    settings_instance = this;
  }
}

Settings::~Settings()
{
  if (settings_instance == this)
    settings_instance = nullptr;
}

Settings& Settings::Instance()
{
  if (!settings_instance)
  {
    LOG_ERROR(logger) << "No lockscreen::Settings created yet.";
  }

  return *settings_instance;
}

}
}
