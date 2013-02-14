// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include "test_mock_scope.h"
#include "config.h"
#include "test_utils.h"

namespace unity
{
namespace dash
{

namespace
{

const gchar* SETTINGS_NAME = "com.canonical.Unity.Dash";
const gchar* SCOPES_SETTINGS_KEY = "scopes";
}

MockGSettingsScopes::MockGSettingsScopes(const char* scopes_settings[])
{
  gsettings_client = g_settings_new(SETTINGS_NAME);
  UpdateScopes(scopes_settings);
}

MockGSettingsScopes::~MockGSettingsScopes()
{
}

void MockGSettingsScopes::UpdateScopes(const char* scopes_settings[])
{
  // Setting the test values
  if (gsettings_client)
    g_settings_set_strv(gsettings_client, SCOPES_SETTINGS_KEY, scopes_settings);
}

}
}