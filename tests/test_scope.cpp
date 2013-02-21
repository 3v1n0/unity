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
  }
 
  void ConnectAndWait()
  {
    scope_->Connect();
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
                         [filter_to_wait_for] { return g_strdup_printf("Filter '%s' not found", filter_to_wait_for.c_str()); });
    return filter_ret;
  }

  Scope::Ptr scope_;
};

TEST_F(TestScope, TestConnection)
{
  ConnectAndWait();
  ASSERT_TRUE(scope_->connected);
}

TEST_F(TestScope, Search)
{
  // Auto-connect on search
  bool search_ok = false;
  auto search_callback = [&search_ok] (glib::HintsMap const&, glib::Error const&) {
    search_ok = true;
  };

  scope_->Search("12:test_search", search_callback, nullptr);
  Utils::WaitUntilMSec(search_ok,
                       2000,
                       [] { return g_strdup("Search did not finish sucessfully"); });
}

TEST_F(TestScope, ActivateUri)
{
  // Auto-connect on activate
  bool activated_return = false;
  auto activate_callback = [&activated_return] (LocalResult const&, ScopeHandledType, glib::Error const&) {
    activated_return = true;
  };

  LocalResult result; result.uri = "file:://test";
  scope_->Activate(result,
                   activate_callback);

  Utils::WaitUntilMSec(activated_return,
                       2000,
                       [] { return g_strdup("Failed to activate"); });
}

TEST_F(TestScope, Preview)
{
  // Auto-connect on preview
  bool preview_ok = false;
  auto preview_callback = [&preview_ok] (LocalResult const&, ScopeHandledType, glib::Error const&) {
    preview_ok = true;
  };

  LocalResult result; result.uri = "file:://test";
  scope_->Preview(result,
                  preview_callback);

  Utils::WaitUntilMSec(preview_ok,
                       2000,
                       [] { return g_strdup("Failed to preview"); });
}

TEST_F(TestScope, ActivatePreviewAction)
{
  // Auto-connect on preview
  bool preview_action_ok = false;
  auto preview_action_callback = [&preview_action_ok] (LocalResult const&, ScopeHandledType, glib::Error const&) {
    preview_action_ok = true;
  };

  LocalResult result; result.uri = "file:://test";
  scope_->ActivatePreviewAction("play",
                                result,
                                glib::HintsMap(),
                                preview_action_callback);

  Utils::WaitUntilMSec(preview_action_ok,
                       2000,
                       [] { return g_strdup("Failed to activate preview action"); });
}

TEST_F(TestScope, UpdatePreviewProperty)
{
  // Auto-connect on preview
  bool update_preview_property_returned = false;
  auto update_property_callback = [&update_preview_property_returned] (glib::HintsMap const&, glib::Error const&) {
    update_preview_property_returned = true;
  };

  LocalResult result; result.uri = "file:://test";
  glib::HintsMap hints;
  hints["test"] = g_variant_new_string("plop");
  scope_->UpdatePreviewProperty(result,
                                hints,
                                update_property_callback,
                                nullptr);

  Utils::WaitUntilMSec(update_preview_property_returned,
                       2000,
                       [] { return g_strdup("Failed to update preview property"); });
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
  scope_->Search("13:test_search1", search_callback);

  Results::Ptr results = scope_->results();
  Utils::WaitUntilMSec(search_ok, 2000, [] { return g_strdup("First search failed."); });
  Utils::WaitUntilMSec([results] { return results->count() == 13; },
                       true,
                       2000,
                       [results] { return g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 13).", static_cast<int>(results->count())); });
  EXPECT_EQ(search_ok, true);

  Results::Ptr category_model0 = scope_->GetResultsForCategory(0);
  Results::Ptr category_model1 = scope_->GetResultsForCategory(1);
  Results::Ptr category_model2 = scope_->GetResultsForCategory(2);

  EXPECT_EQ(category_model0->count(), 5) << "Category 0 result count not as expected (" << category_model0->count() << " != 5)";
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

  // Results should be updated for fulter.
  Utils::WaitUntilMSec([results] { return results->count() == 4; },
                       true,
                       2000,
                       [results] { return g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 4).", static_cast<int>(results->count())); });
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
