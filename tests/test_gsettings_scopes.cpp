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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include <gtest/gtest.h>
#include <glib-object.h>
#include "test_mock_scope.h"
#include "test_utils.h"

#include "config.h"

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace
{

const gchar* SCHEMA_DIRECTORY = BUILDDIR"/settings";

const gchar* SETTINGS_NAME = "com.canonical.Unity.Dash";
const gchar* SCOPES_SETTINGS_KEY = "scopes";
const gchar* ALWAYS_SETTINGS_KEY = "always-search";


const char* scopes_default[] =  { "testscope1.scope",
                                  "testscope2.scope",
                                  "testscope3.scope",
                                  "testscope4.scope",
                                  NULL
                                };
const char* scopes_2removed[] = { "testscope1.scope",
                                  "testscope3.scope",
                                  "testscope4.scope",
                                  NULL
                                };

const char* always_create[] = { "testscope1.scope",
                                "testscope2.scope",
                                NULL
                              };

// A new one of these is created for each test
class TestGSettingsScopes : public testing::Test
{
public:
  TestGSettingsScopes():scope_added(0),scope_removed(0),scopes_reordered(0)
  {}

  virtual void SetUp()
  {
    // set the data directory so gsettings can find the schema
    g_setenv("GSETTINGS_SCHEMA_DIR", SCHEMA_DIRECTORY, true);
    g_setenv("GSETTINGS_BACKEND", "memory", true);

    // Setting the test values
    gsettings_client = g_settings_new(SETTINGS_NAME);
    g_settings_set_strv(gsettings_client, SCOPES_SETTINGS_KEY, scopes_default);
    g_settings_set_strv(gsettings_client, ALWAYS_SETTINGS_KEY, always_create);
  }

  virtual void TearDown()
  {
    g_setenv("GSETTINGS_SCHEMA_DIR", "", true);
    g_setenv("GSETTINGS_BACKEND", "", true);
  }

  void ConnectScope(Scopes* scopes)
  {
    scopes->scope_added.connect([this](Scope::Ptr const& scope, int)
    {
      ++scope_added;
    });
    scopes->scope_removed.connect([this](Scope::Ptr const& scope)
    {
      ++scope_removed;
    });
    scopes->scopes_reordered.connect([this](Scopes::ScopeList const& list)
    {
      ++scopes_reordered;
    });
  }

  int scope_added;
  int scope_removed;
  int scopes_reordered;

  glib::Object<GSettings> gsettings_client;
};



TEST_F(TestGSettingsScopes, TestConstruction)
{
  MockGSettingsScopes scopes;
}

TEST_F(TestGSettingsScopes, TestLoad)
{
  MockGSettingsScopes scopes;
  ConnectScope(&scopes);

  scopes.LoadScopes();

  EXPECT_EQ(scope_added, (unsigned int)4);

  int position = -1;
  EXPECT_TRUE(scopes.GetScope("testscope1.scope", &position) && position == 0);
  EXPECT_TRUE(scopes.GetScope("testscope2.scope", &position) && position == 1);
  EXPECT_TRUE(scopes.GetScope("testscope3.scope", &position) && position == 2);
  EXPECT_TRUE(scopes.GetScope("testscope4.scope", &position) && position == 3);
}

TEST_F(TestGSettingsScopes, TestScopesAdded)
{
  MockGSettingsScopes scopes;
  ConnectScope(&scopes);

  scopes.InsertScope("testscope1.scope", 0);
  EXPECT_EQ(scopes.count, (unsigned int)1);
  EXPECT_EQ(scopes_reordered, (unsigned int)0);

  // add another
  scopes.InsertScope("testscope2.scope", 1);
  EXPECT_EQ(scopes.count, (unsigned int)2);
  EXPECT_EQ(scopes_reordered, (unsigned int)0);

  int position = -1;
  EXPECT_TRUE(scopes.GetScope("testscope1.scope", &position) && position == 0);
  EXPECT_TRUE(scopes.GetScope("testscope2.scope", &position) && position == 1);

  // add another
  scopes.InsertScope("testscope2.scope", 0);

  EXPECT_EQ(scopes.count, (unsigned int)2);
  EXPECT_EQ(scopes_reordered, (unsigned int)1);

  EXPECT_TRUE(scopes.GetScope("testscope1.scope", &position) && position == 1);
  EXPECT_TRUE(scopes.GetScope("testscope2.scope", &position) && position == 0);
}

TEST_F(TestGSettingsScopes, TestScopesAddSame)
{
  MockGSettingsScopes scopes;
  ConnectScope(&scopes);

  scopes.InsertScope("testscope1.scope", 0);
  EXPECT_EQ(scope_added, (unsigned int)1);
  EXPECT_EQ(scopes.count, (unsigned int)1);

  // shouldnt add another
  scopes.InsertScope("testscope1.scope", 1);

  EXPECT_EQ(scope_added, (unsigned int)1);
  EXPECT_EQ(scopes.count, (unsigned int)1);
  EXPECT_EQ(scopes_reordered, (unsigned int)0);
}

TEST_F(TestGSettingsScopes, TestScopesRemove)
{
  MockGSettingsScopes scopes;
  ConnectScope(&scopes);
  
  scopes.LoadScopes();

  EXPECT_EQ(scope_added, (unsigned int)4);
  EXPECT_EQ(scopes.count, (unsigned int)4);

  g_settings_set_strv(gsettings_client, SCOPES_SETTINGS_KEY, scopes_2removed);
  Utils::WaitUntilMSec([this] { return scope_removed > 0; }, true, 2000);

  EXPECT_EQ(scope_removed, (unsigned int)1);
  EXPECT_EQ(scopes.count, (unsigned int)3);
}


}
