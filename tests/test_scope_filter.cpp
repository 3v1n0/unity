// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include <boost/lexical_cast.hpp>
#include <gtest/gtest.h>
#include <glib-object.h>
#include <unity-protocol.h>

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Scope.h>
#include <UnityCore/Variant.h>

#include <UnityCore/CheckOptionFilter.h>
#include <UnityCore/MultiRangeFilter.h>
#include <UnityCore/RadioOptionFilter.h>
#include <UnityCore/RatingsFilter.h>

#include "test_utils.h"

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace unity
{
namespace dash
{

namespace
{
const std::string SCOPE_NAME = "testscope1.scope";
}

class TestScopeFilter : public ::testing::Test
{
public:
  TestScopeFilter() { }

  virtual void SetUp()
  {
    glib::Error err;
    ScopeData::Ptr data(ScopeData::ReadProtocolDataForId(SCOPE_NAME, err));
    ASSERT_TRUE(err ? false : true);

    scope_.reset(new Scope(data));
    scope_->Init();
    ConnectAndWait();
  }
 
  void ConnectAndWait()
  {
    scope_->Connect();
    Utils::WaitUntilMSec([this] { return scope_->connected() == true; }, true, 2000);
  }

  void WaitForSynchronize(Filters::Ptr const& model, unsigned int count)
  {
    Utils::WaitUntil([model,count] { return model->count == count; });
  }

  Scope::Ptr scope_;
};

TEST_F(TestScopeFilter, TestFilterCheckOption)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  CheckOptionFilter::Ptr filter = static_pointer_cast<CheckOptionFilter>(filters->FilterAtIndex(0));
  EXPECT_EQ(filter->id, "categories");
  EXPECT_EQ(filter->name, "Categories");
  EXPECT_EQ(filter->icon_hint, "");
  EXPECT_EQ(filter->renderer_name, "filter-checkoption");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  CheckOptionFilter::CheckOptions options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)3);
  
  EXPECT_EQ(options[0]->id, "cat0");
  EXPECT_EQ(options[0]->name, "Category 0");
  EXPECT_EQ(options[0]->icon_hint, "gtk-cdrom");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "cat1");
  EXPECT_EQ(options[1]->name, "Category 1");
  EXPECT_EQ(options[1]->icon_hint, "gtk-directory");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "cat2");
  EXPECT_EQ(options[2]->name, "Category 2");
  EXPECT_EQ(options[2]->icon_hint, "gtk-clear");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestScopeFilter, TestFilterCheckOptionLogic)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  CheckOptionFilter::Ptr filter = static_pointer_cast<CheckOptionFilter>(filters->FilterAtIndex(0));
  CheckOptionFilter::CheckOptions options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  options[0]->active = false;
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[1]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[2]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_TRUE (options[2]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
}

TEST_F(TestScopeFilter, TestFilterRadioOption)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  RadioOptionFilter::Ptr filter = static_pointer_cast<RadioOptionFilter>(filters->FilterAtIndex(1));
  EXPECT_EQ(filter->id, "when");
  EXPECT_EQ(filter->name, "When");
  EXPECT_EQ(filter->icon_hint, "");
  EXPECT_EQ(filter->renderer_name, "filter-radiooption");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  RadioOptionFilter::RadioOptions options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)3);
  
  EXPECT_EQ(options[0]->id, "today");
  EXPECT_EQ(options[0]->name, "Today");
  EXPECT_EQ(options[0]->icon_hint, "");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "yesterday");
  EXPECT_EQ(options[1]->name, "Yesterday");
  EXPECT_EQ(options[1]->icon_hint, "");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "lastweek");
  EXPECT_EQ(options[2]->name, "Last Week");
  EXPECT_EQ(options[2]->icon_hint, "");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestScopeFilter, TestFilterRadioOptionLogic)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  RadioOptionFilter::Ptr filter = static_pointer_cast<RadioOptionFilter>(filters->FilterAtIndex(1));
  RadioOptionFilter::RadioOptions options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  options[0]->active = false;
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[0]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[1]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);

  options[2]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_TRUE (options[2]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
}

TEST_F(TestScopeFilter, TestFilterRatings)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  RatingsFilter::Ptr filter = static_pointer_cast<RatingsFilter>(filters->FilterAtIndex(2));
  EXPECT_EQ(filter->id, "ratings");
  EXPECT_EQ(filter->name, "Ratings");
  EXPECT_EQ(filter->icon_hint, "");
  std::string tmp = filter->renderer_name;
  EXPECT_EQ(filter->renderer_name, "filter-ratings");
  EXPECT_TRUE(filter->visible);
  EXPECT_FALSE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  EXPECT_FLOAT_EQ(filter->rating, 0.0f);
  filter->rating = 0.5f;
  EXPECT_FLOAT_EQ(filter->rating, 0.5f);
}

TEST_F(TestScopeFilter, TestFilterMultiRange)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  MultiRangeFilter::Ptr filter = static_pointer_cast<MultiRangeFilter>(filters->FilterAtIndex(3));
  EXPECT_EQ(filter->id, "size");
  EXPECT_EQ(filter->name, "Size");
  EXPECT_EQ(filter->icon_hint, "");
  std::string tmp = filter->renderer_name;
  EXPECT_EQ(filter->renderer_name, "filter-multirange");
  EXPECT_TRUE(filter->visible);
  EXPECT_TRUE(filter->collapsed);
  EXPECT_FALSE(filter->filtering);

  MultiRangeFilter::Options options = filter->options;
  EXPECT_EQ(options.size(), (unsigned int)4);
  
  EXPECT_EQ(options[0]->id, "1MB");
  EXPECT_EQ(options[0]->name, "1MB");
  EXPECT_EQ(options[0]->icon_hint, "");
  EXPECT_FALSE(options[0]->active);

  EXPECT_EQ(options[1]->id, "10MB");
  EXPECT_EQ(options[1]->name, "10MB");
  EXPECT_EQ(options[1]->icon_hint, "");
  EXPECT_FALSE(options[1]->active);

  EXPECT_EQ(options[2]->id, "100MB");
  EXPECT_EQ(options[2]->name, "100MB");
  EXPECT_EQ(options[2]->icon_hint, "");
  EXPECT_FALSE(options[2]->active);
}

TEST_F(TestScopeFilter, TestFilterMultiRangeLogic)
{
  Filters::Ptr filters = scope_->filters;
  WaitForSynchronize(filters, 4);

  MultiRangeFilter::Ptr filter = static_pointer_cast<MultiRangeFilter>(filters->FilterAtIndex(3));
  MultiRangeFilter::Options options = filter->options;

  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  options[0]->active = true;
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (filter->filtering);
  options[3]->active = true;
  EXPECT_FALSE (options[0]->active);
  EXPECT_TRUE (options[3]->active);

  options[0]->active = true;
  options[1]->active = true;
  EXPECT_TRUE (filter->filtering);
  EXPECT_TRUE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  options[0]->active = false;
  EXPECT_TRUE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_TRUE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);

  filter->Clear();
  EXPECT_FALSE (filter->filtering);
  EXPECT_FALSE (options[0]->active);
  EXPECT_FALSE (options[1]->active);
  EXPECT_FALSE (options[2]->active);
  EXPECT_FALSE (options[3]->active);
}

}
}
