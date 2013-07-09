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
    FakePlacesGroup()
      : PlacesGroup(dash::Style::Instance())
      , is_expanded_(false)
    {}

    bool GetExpanded() const override { return is_expanded_; }
    void SetExpanded(bool is_expanded) override { is_expanded_ = is_expanded; expanded.emit(this); }

    using PlacesGroup::_using_filters_background;
    bool is_expanded_;
  };

  struct FakeScopeView : public ScopeView
  {
    FakeScopeView(MockScope::Ptr const& scope)
      : ScopeView(scope, nullptr)
    {}

    using ScopeView::OnResultAdded;
    using ScopeView::search_string_;

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
  Utils::WaitUntilMSec([this] { return scope_view_->GetOrderedCategoryViews().size() == 2; });
  ASSERT_EQ(scope_view_->GetOrderedCategoryViews().size(), 2);
}

TEST_F(TestScopeView, TestFilterExpansion)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(4);
  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 4);

  scope_view_->filters_expanded = true;
  for (unsigned i = 0; i < scope_view_->fake_groups_.size(); ++i)
    EXPECT_EQ(scope_view_->fake_groups_[i]->_using_filters_background, true);
}

TEST_F(TestScopeView, TestCategoryExpansion_OneCategory_EmptySearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(1);
  scope_view_->search_string_ = "";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 1);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_OneCategory_FilledSearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(1);
  scope_view_->search_string_ = "Ubuntu";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 1);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_TwoCategory_EmptySearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(2);
  scope_view_->search_string_ = "";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 2);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_TwoCategory_FilledSearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(2);
  scope_view_->search_string_ = "Ubuntu";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 2);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_ThreeCategory_EmptySearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(3);
  scope_view_->search_string_ = "";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 3);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_ThreeCategory_FilledSearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(3);
  scope_view_->search_string_ = "Ubuntu";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 3);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_TwoCategory_OnResultAdded_EmptySearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(2);

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 2);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });

  /* XXX: we should emit the signal not call the callback */
  MockResults added_results(2);

  scope_view_->fake_groups_[0]->SetExpanded(true);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });

  scope_view_->OnResultAdded(added_results.RowAtIndex(0));
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_TwoCategory_OnResultAdded_FilledSearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(2);
  scope_view_->search_string_ = "Ubuntu";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 2);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });

  /* XXX: we should emit the signal not call the callback */
  MockResults added_results(2);

  scope_view_->fake_groups_[0]->SetExpanded(true);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });

  scope_view_->OnResultAdded(added_results.RowAtIndex(0));
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[1]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_ThreeCategory_OnResultAdded_EmptySearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(3);

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 3);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  /* XXX: we should emit the signal not call the callback */
  MockResults added_results(2);
  scope_view_->OnResultAdded(added_results.RowAtIndex(0));
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  scope_view_->fake_groups_[0]->SetExpanded(true);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  scope_view_->OnResultAdded(added_results.RowAtIndex(1));
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });
}

TEST_F(TestScopeView, TestCategoryExpansion_ThreeCategory_OnResultAdded_FilledSearchString)
{
  MockCategories::Ptr categories = std::make_shared<MockCategories>(3);
  scope_view_->search_string_ = "Ubuntu";

  scope_->categories.changed.emit(categories);
  ASSERT_EQ(scope_view_->fake_groups_.size(), 3);
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  /* XXX: we should emit the signal not call the callback */
  MockResults added_results(2);
  scope_view_->OnResultAdded(added_results.RowAtIndex(0));
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  scope_view_->fake_groups_[0]->SetExpanded(true);
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });

  scope_view_->OnResultAdded(added_results.RowAtIndex(1));
  Utils::WaitUntilMSec([this] () { return scope_view_->fake_groups_[0]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[1]->GetExpanded(); });
  Utils::WaitUntilMSec([this] () { return not scope_view_->fake_groups_[2]->GetExpanded(); });
}

}
}
