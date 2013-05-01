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
#include <dash/ScopeView.h>
#include <dash/PlacesGroup.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/UnitySettings.h>
#include <UnityCore/Category.h>

#include "MockCategories.h"
#include "test_mock_scope.h"
#include "test_utils.h"

namespace unity
{
namespace dash
{

struct TestScopeView : public ::testing::Test
{
  struct FakePlacesGroup : public PlacesGroup
  {
    FakePlacesGroup():PlacesGroup(dash::Style::Instance()) {}

    using PlacesGroup::_using_filters_background;
  };

  struct FakeScopeView : public ScopeView
  {
    FakeScopeView(MockScope::Ptr const& scope)
      : ScopeView(scope, nullptr)
    {}

    using ScopeView::OnCategoryAdded;

    PlacesGroup::Ptr CreatePlacesGroup(Category const& category) override
    {
      FakePlacesGroup* group = new FakePlacesGroup();
      fake_groups_.push_back(group);
      return PlacesGroup::Ptr(group);
    }

    std::vector<FakePlacesGroup*> fake_groups_;
  };

  TestScopeView()
    : scope_data_(std::make_shared<MockScopeData>(""))
    , scope_(std::make_shared<MockScope>(scope_data_, "", "", 10))
    , scope_view_(new FakeScopeView(scope_))
  {}

  unity::Settings settings;
  dash::Style style;
  MockScopeData::Ptr scope_data_;
  MockScope::Ptr scope_;
  std::unique_ptr<FakeScopeView> scope_view_;
};

TEST_F(TestScopeView, TestCategoryInsert)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(2);
  scope_->categories.changed.emit(categories);

  EXPECT_EQ(scope_view_->GetOrderedCategoryViews().size(), 2);
}

TEST_F(TestScopeView, TestFilterExpansion)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(4);
  scope_->categories.changed.emit(categories);

  EXPECT_EQ(scope_view_->fake_groups_.size(), 4);

  scope_view_->filters_expanded = true;
  for (unsigned i = 0; i < scope_view_->fake_groups_.size(); ++i)
    EXPECT_EQ(scope_view_->fake_groups_[i]->_using_filters_background, true);
}

TEST_F(TestScopeView, TestCategoryExpansion)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(1);
  scope_->categories.changed.emit(categories);

  EXPECT_EQ(scope_view_->fake_groups_.size(), 1);
  Utils::WaitUntil([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });

  //Utils::WaitUntil([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });
}

}
}
