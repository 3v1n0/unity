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

using namespace unity;
using namespace unity::dash;

namespace unity
{
namespace dash
{

class TestScopeView : public ::testing::Test
{
public:

  class FakePlacesGroup : public PlacesGroup
  {
  public:
    FakePlacesGroup():PlacesGroup(dash::Style::Instance()) {}

    using PlacesGroup::_using_filters_background;
  };

  class FakeScopeView : public ScopeView
  {
  public:
    FakeScopeView():ScopeView(Scope::Ptr(), nullptr) {}

    using ScopeView::OnCategoryAdded;

    virtual PlacesGroup::Ptr CreatePlacesGroup(Category const& category)
    {
      FakePlacesGroup* group = new FakePlacesGroup();
      fake_groups_.push_back(group);
      return PlacesGroup::Ptr(group);
    }

    std::vector<FakePlacesGroup*> fake_groups_;
  };

  TestScopeView()
    : scope_view_(new FakeScopeView())
    , categories_(5)
  {
  }

  unity::Settings settings;
  dash::Style style;
  std::unique_ptr<FakeScopeView> scope_view_;
  MockCategories categories_;
};

TEST_F(TestScopeView, TestCategoryInsert)
{
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(0));
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(1));

  EXPECT_EQ(scope_view_->GetOrderedCategoryViews().size(), 2);
}

TEST_F(TestScopeView, TestFilterExpansion)
{
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(0));
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(1));
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(2));
  scope_view_->OnCategoryAdded(categories_.RowAtIndex(3));

  EXPECT_EQ(scope_view_->fake_groups_.size(), 4);

  scope_view_->filters_expanded = true;
  for (unsigned i = 0; i < scope_view_->fake_groups_.size(); i++)
  {
    EXPECT_EQ(scope_view_->fake_groups_[i]->_using_filters_background, true);
  }
}

}
}
