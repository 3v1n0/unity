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

#include <NuxCore/Logger.h>


DECLARE_LOGGER(logger, "unity.appmanager.bamf");


namespace unity
{
// This function is used by the static Default method on the ApplicationManager
// class, and uses link time to make sure there is a function available.
ApplicationManagerPtr create_application_manager()
{
    return ApplicationManagerPtr(new BamfApplicationManager());
}

BamfApplication::BamfApplication()
{
}

BamfApplication::~BamfApplication()
{
}

std::string BamfApplication::icon() const
{
    return "TODO";
}

std::string BamfApplication::title() const
{
    return "TODO";
}


BamfApplicationManager::BamfApplicationManager()
{
    LOG_TRACE(logger) << "Create BamfApplicationManager";
}

BamfApplicationManager::~BamfApplicationManager()
{
}

ApplicationPtr BamfApplicationManager::active_application() const
{
    return ApplicationPtr();
}


} // namespace unity
