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

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace
{


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

// A new one of these is created for each test
class TestGSettingsScopes : public testing::Test
{
public:
  TestGSettingsScopes():scope_added(0),scope_removed(0),scopes_reordered(0)
  {}

  virtual void SetUp() { Utils::init_gsettings_test_environment(); }
  virtual void TearDown() { Utils::reset_gsettings_test_environment(); }

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
};



TEST_F(TestGSettingsScopes, TestConstruction)
{
  MockGSettingsScopes scopes(scopes_default);
}

TEST_F(TestGSettingsScopes, TestLoad)
{
  MockGSettingsScopes scopes(scopes_default);
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
  MockGSettingsScopes scopes(scopes_default);
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
  MockGSettingsScopes scopes(scopes_default);
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
  MockGSettingsScopes scopes(scopes_default);
  ConnectScope(&scopes);
  
  scopes.LoadScopes();

  EXPECT_EQ(scope_added, (unsigned int)4);
  EXPECT_EQ(scopes.count, (unsigned int)4);

  scopes.UpdateScopes(scopes_2removed);
  Utils::WaitUntilMSec([this] { return scope_removed > 0; }, true, 2000);

  EXPECT_EQ(scope_removed, (unsigned int)1);
  EXPECT_EQ(scopes.count, (unsigned int)3);
}


}
