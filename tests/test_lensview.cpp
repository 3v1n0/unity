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
#include <dash/LensView.h>
#include <dash/PlacesGroup.h>
#include <unity-shared/DashStyle.h>
#include <unity-shared/UnitySettings.h>
#include <UnityCore/Category.h>

using namespace unity;
using namespace unity::dash;

namespace unity
{
namespace dash
{

class TestLensView : public ::testing::Test
{
public:

  class FakePlacesGroup : public PlacesGroup
  {
  public:
    FakePlacesGroup():PlacesGroup(dash::Style::Instance()) {}

    using PlacesGroup::_using_filters_background;
  };

  class FakeLensView : public LensView
  {
  public:
    FakeLensView():LensView(Lens::Ptr(), nullptr) {}

    using LensView::OnCategoryAdded;

    PlacesGroup* CreatePlacesGroup()
    {
      FakePlacesGroup* category = new FakePlacesGroup();
      fake_categories_.push_back(category);
      return category;
    }

    std::vector<FakePlacesGroup*> fake_categories_;
  };

  TestLensView()
    : lens_view_(new FakeLensView())
  {
  }

  unity::Settings settings;
  dash::Style style;
  std::unique_ptr<FakeLensView> lens_view_;
};

TEST_F(TestLensView, TestCategoryInsert)
{
  Category cat(NULL, NULL, NULL);
  lens_view_->OnCategoryAdded(cat);

  ASSERT_TRUE(lens_view_->categories().size() > 0);
}

TEST_F(TestLensView, TestFilterExpansion)
{
  Category cat(NULL, NULL, NULL);
  lens_view_->OnCategoryAdded(cat);
  lens_view_->OnCategoryAdded(cat);
  lens_view_->OnCategoryAdded(cat);

  lens_view_->filters_expanded = true;
  for (unsigned i = 0; i < lens_view_->fake_categories_.size(); i++)
  {
    EXPECT_EQ(lens_view_->fake_categories_[i]->_using_filters_background, true);
  }
}

}
}
