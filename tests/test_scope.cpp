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
 #include <UnityCore/CheckOptionFilter.h>

#include "test_utils.h"
#include "RadioOptionFilter.h"

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

class TestScope : public ::testing::Test
{
public:
  TestScope() { }

  virtual void SetUp()
  {
    glib::Error err;
    ScopeData::Ptr data(ScopeData::ReadProtocolDataForId(SCOPE_NAME, err));
    ASSERT_TRUE(err ? false : true);

    scope_.reset(new Scope(data));
    scope_->Init();
    scope_->Connect();

    WaitForConnected();
  }
 
  void WaitForConnected()
  {
    Utils::WaitUntilMSec([this] { return scope_->connected() == true; }, true, 2000);
  }

  Filter::Ptr WaitForFilter(std::string const& filter_to_wait_for)
  {
    Filter::Ptr filter_ret;
    Filters::Ptr filters = scope_->filters();    
    Utils::WaitUntilMSec([filters, filter_to_wait_for, &filter_ret]
                         {
                           for (std::size_t i = 0; i < filters->count(); i++)
                           {
                             Filter::Ptr filter = filters->FilterAtIndex(i);
                             if (filter && filter->id == filter_to_wait_for)
                             {
                               filter_ret = filter;
                               return true;
                             }
                           }
                           return false;
                         },
                         true,
                         2000,
                         glib::String(g_strdup_printf("Filter '%s' not found", filter_to_wait_for.c_str())));
    return filter_ret;
  }

  Scope::Ptr scope_;
};

TEST_F(TestScope, TestConnection)
{
  ASSERT_TRUE(scope_->connected);
}

TEST_F(TestScope, UpdateSearchCategoryWorkflow)
{
  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (glib::HintsMap const&, glib::Error const& error) {
    search_finished = true;
    search_ok = error ? false : true;
  };

  // 1. First search
  scope_->Search("12:test_search1", search_callback);

  Results::Ptr results = scope_->results();
  Utils::WaitUntilMSec(search_ok, 2000, glib::String(g_strdup("First search failed.")));
  Utils::WaitUntilMSec([results] { return results->count() == 12; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 12).", static_cast<int>(results->count()))));
  EXPECT_EQ(search_ok, true);

  Results::Ptr category_model0 = scope_->GetResultsForCategory(0);
  Results::Ptr category_model1 = scope_->GetResultsForCategory(1);
  Results::Ptr category_model2 = scope_->GetResultsForCategory(2);

  EXPECT_EQ(category_model0->count(), 4) << "Category 0 result count not as expected (" << category_model0->count() << " != 4)";
  EXPECT_EQ(category_model1->count(), 4) << "Category 1 result count not as expected (" << category_model1->count() << " != 4)";
  EXPECT_EQ(category_model2->count(), 4) << "Category 2 result count not as expected (" << category_model2->count() << " != 4)";

  // 2. Update the filter.
  CheckOptionFilter::Ptr type_filter = std::static_pointer_cast<CheckOptionFilter>(WaitForFilter("categories"));
  ASSERT_TRUE(type_filter ? true : false);

  bool filter_updated = false;
  std::vector<FilterOption::Ptr> options = type_filter->options();
  for (FilterOption::Ptr const& option : options)
  {
    if (option->id == "cat1")
    {
      option->active = true;
      filter_updated = true;
    }
  }
  EXPECT_TRUE(filter_updated) << "Could not update filter opiton 'cat1' of filter 'categories'";

  // 1. Second search - Filtered by 'cat1'
  scope_->Search("12:test_search2", search_callback);

  Utils::WaitUntilMSec(search_ok, 2000, glib::String(g_strdup("First search failed.")));
  Utils::WaitUntilMSec([results] { return results->count() == 4; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 4).", static_cast<int>(results->count()))));
  EXPECT_EQ(search_ok, true);

  category_model0 = scope_->GetResultsForCategory(0);
  category_model1 = scope_->GetResultsForCategory(1);
  category_model2 = scope_->GetResultsForCategory(2);

  EXPECT_EQ(category_model0->count(), 0) << "Category 0 result count not as expected (" << category_model0->count() << " != 0)";
  EXPECT_EQ(category_model1->count(), 4) << "Category 1 result count not as expected (" << category_model1->count() << " != 4)";
  EXPECT_EQ(category_model2->count(), 0) << "Category 2 result count not as expected (" << category_model2->count() << " != 0)";

}

} // namespace dash
} // namespace unity
