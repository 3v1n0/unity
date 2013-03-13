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
#include "test_utils.h"
#include "UnityCore/ScopeData.cpp"

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace
{

// A new one of these is created for each test
class TestScopeData : public testing::Test
{
public:
  TestScopeData()
  {}
};

TEST_F(TestScopeData, TestReadExisting)
{
  glib::Error error;
  ScopeData::Ptr scope_data(ScopeData::ReadProtocolDataForId("testscope1.scope", error));
  ASSERT_TRUE(scope_data && !error);

  EXPECT_EQ(scope_data->id(),                  "testscope1.scope");
  EXPECT_EQ(scope_data->name(),                "TestScope1");
  EXPECT_EQ(scope_data->dbus_name(),           "com.canonical.Unity.Test.Scope");
  EXPECT_EQ(scope_data->dbus_path(),           "/com/canonical/unity/scope/testscope1");
  EXPECT_EQ(scope_data->icon_hint(),           "/usr/share/unity/6/icon-sub1.svg");
  EXPECT_EQ(scope_data->category_icon_hint(),  "");
  EXPECT_EQ(scope_data->type(),                "varia");
  EXPECT_EQ(scope_data->query_pattern(),       "^@");
  EXPECT_EQ(scope_data->description(),         "Find various stuff 1");
  EXPECT_EQ(scope_data->shortcut(),            "q");
  EXPECT_EQ(scope_data->search_hint(),         "Search stuff 1");
  EXPECT_TRUE(scope_data->keywords().size() == 1 && scope_data->keywords().front()=="misc");
  // EXPECT_EQ(scope_data->full_path(),           "");
}

TEST_F(TestScopeData, TestNonExisting)
{
  glib::Error error;
  ScopeData::Ptr scope_data(ScopeData::ReadProtocolDataForId("non-existing.scope", error));
  EXPECT_TRUE(scope_data && error);
}

}
