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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <dash/ScopeBar.h>

#include "unity-shared/DashStyle.h"
#include "unity-shared/UnitySettings.h"
#include "test_mock_scope.h"

using namespace unity;
using namespace unity::dash;

namespace unity
{
namespace dash
{

class TestScopeBar : public ::testing::Test
{
public:
  TestScopeBar()
  {
  }

  void CheckSize(ScopeBar const& scope_bar, int size)
  {
    EXPECT_EQ(scope_bar.icons_.size(), size);
  }

  unity::Settings settings;
  dash::Style style;
};

TEST_F(TestScopeBar, TestAddScopes)
{
  ScopeBar scope_bar;

  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope1.scope"), "TestScope1", "icon-sub1.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope2.scope"), "TestScope2", "icon-sub2.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope3.scope"), "TestScope3", "icon-sub3.svg"));

  CheckSize(scope_bar, 3);
}

TEST_F(TestScopeBar, TestActivate)
{
  ScopeBar scope_bar;

  std::string active_scope = "";
  scope_bar.scope_activated.connect([&active_scope](std::string const& activated) { active_scope = activated; } );

  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope1.scope"), "TestScope1", "icon-sub1.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope2.scope"), "TestScope2", "icon-sub2.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope3.scope"), "TestScope3", "icon-sub3.svg"));

  scope_bar.Activate("testscope1.scope");
  EXPECT_EQ(active_scope, "testscope1.scope");

  scope_bar.Activate("testscope2.scope");
  EXPECT_EQ(active_scope, "testscope2.scope");

  scope_bar.Activate("testscope3.scope");
  EXPECT_EQ(active_scope, "testscope3.scope");
}

TEST_F(TestScopeBar, TestActivateNext)
{
  ScopeBar scope_bar;

  std::string active_scope = "";
  scope_bar.scope_activated.connect([&active_scope](std::string const& activated) { active_scope = activated; } );

  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope1.scope"), "TestScope1", "icon-sub1.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope2.scope"), "TestScope2", "icon-sub2.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope3.scope"), "TestScope3", "icon-sub3.svg"));

  scope_bar.ActivateNext();
  EXPECT_EQ(active_scope, "testscope1.scope");

  scope_bar.ActivateNext();
  EXPECT_EQ(active_scope, "testscope2.scope");

  scope_bar.ActivateNext();
  EXPECT_EQ(active_scope, "testscope3.scope");

  scope_bar.ActivateNext();
  EXPECT_EQ(active_scope, "testscope1.scope");
}

TEST_F(TestScopeBar, TestActivatePrevious)
{
  ScopeBar scope_bar;

  std::string active_scope = "";
  scope_bar.scope_activated.connect([&active_scope](std::string const& activated) { active_scope = activated; } );

  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope1.scope"), "TestScope1", "icon-sub1.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope2.scope"), "TestScope2", "icon-sub2.svg"));
  scope_bar.AddScope(std::make_shared<MockScope>(std::make_shared<MockScopeData>("testscope3.scope"), "TestScope3", "icon-sub3.svg"));

  scope_bar.ActivatePrevious();
  EXPECT_EQ(active_scope, "testscope3.scope");

  scope_bar.ActivatePrevious();
  EXPECT_EQ(active_scope, "testscope2.scope");

  scope_bar.ActivatePrevious();
  EXPECT_EQ(active_scope, "testscope1.scope");

  scope_bar.ActivatePrevious();
  EXPECT_EQ(active_scope, "testscope3.scope");
}


}
}
