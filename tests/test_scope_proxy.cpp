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
#include <UnityCore/ScopeProxy.h>

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
static const string scope_name = "com.canonical.Unity.Test";
static const string scope_path = "/com/canonical/unity/scope/testscope1";
}

class TestScopeProxy : public ::testing::Test
{
public:
  TestScopeProxy()
  {
    scope_proxy_.reset(new ScopeProxy(scope_name, scope_path));
  }
 
  void ConnectAndWait()
  {
    scope_proxy_->CreateProxy();
    Utils::WaitUntilMSec([this] { return scope_proxy_->connected == true; },
                         true,
                         2000,
                         glib::String(g_strdup("Could not connect to proxy")));
  }

  ScopeProxyInterface::Ptr scope_proxy_;
};

TEST_F(TestScopeProxy, TestConnection)
{
  ConnectAndWait();

  ASSERT_TRUE(scope_proxy_->connected);
}

TEST_F(TestScopeProxy, TestFilterSync)
{
  ConnectAndWait();

  bool filter_changed = false;
  scope_proxy_->filters.changed.connect([&filter_changed] (Filters::Ptr const& filters) { filter_changed = true; });
  Filters::Ptr filters = scope_proxy_->filters();
  Utils::WaitUntilMSec([filters] { return filters->count() > 0; },
                       true,
                       3000,
                       glib::String(g_strdup("Filters coutn = 0")));

  FilterAdaptor adaptor = filters->RowAtIndex(0);

  Filter::Ptr filter = filters->FilterAtIndex(0);
  EXPECT_TRUE(filter ? true : false);
  EXPECT_EQ(filter->renderer_name, "filter-radiooption");

  RadioOptionFilter::Ptr radio_option_filter = std::static_pointer_cast<RadioOptionFilter>(filter);
  RadioOptionFilter::RadioOptions radio_opitons = radio_option_filter->options();
  EXPECT_TRUE(radio_opitons.size() > 0);
}

TEST_F(TestScopeProxy, TestCategorySync)
{
  ConnectAndWait();

  Categories::Ptr categories = scope_proxy_->categories();
  Utils::WaitUntilMSec([categories] { return categories->count() > 0; },
                       true,
                       3000,
                       glib::String(g_strdup("Cateogry count = 0")));
}

TEST_F(TestScopeProxy, TestSearch)
{
  // Auto-connect on search

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  };

  scope_proxy_->Search("12:test_search", search_callback, nullptr);

  Results::Ptr results = scope_proxy_->results();
  Utils::WaitUntilMSec([&, results] { return search_finished == true && results->count() == 12; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("Either search didn't finish, or result count is not as expected (%u != 12).", static_cast<unsigned>(results->count()))));
  EXPECT_TRUE(search_ok == true);
}

// Test that searching twice will not concatenate results.
TEST_F(TestScopeProxy, TestMultiSearch)
{
  // Auto-connect on search

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  };

  // First Search
  scope_proxy_->Search("12:test_search", search_callback, nullptr);

  Results::Ptr results = scope_proxy_->results();
  Utils::WaitUntilMSec([&, results] { return search_finished == true && results->count() == 12; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 12).", static_cast<int>(results->count()))));
  EXPECT_TRUE(search_ok == true);

  // Second Search
  search_ok = false;
  search_finished = false;
  scope_proxy_->Search("5:test_search", search_callback, nullptr);
  
  Utils::WaitUntilMSec([&search_finished, results] { return search_finished == true && results->count() == 5; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("Second search. Either search didn't finish, or result count is not as expected (%u != 5).", static_cast<unsigned>(results->count()))));
  EXPECT_TRUE(search_ok == true);
}

TEST_F(TestScopeProxy, TestSearchCategories)
{
  // Auto-connect on search

  scope_proxy_->Search("12:test_search", [&] (glib::HintsMap const&, glib::Error const& error) { }, nullptr);

  Results::Ptr results = scope_proxy_->results();
  Utils::WaitUntilMSec([results] { return results->count() == 12; },
                       true,
                       2000,
                       glib::String(g_strdup_printf("Result count is not as expected (%u != 12).", static_cast<unsigned>(results->count()))));

  Results::Ptr category_model0 = scope_proxy_->GetResultsForCategory(0);
  Results::Ptr category_model1 = scope_proxy_->GetResultsForCategory(1);
  Results::Ptr category_model2 = scope_proxy_->GetResultsForCategory(2);
  Results::Ptr category_model3 = scope_proxy_->GetResultsForCategory(3);

  EXPECT_EQ(category_model0->count(), 4) << "Category 0 result count not as expected (" << category_model0->count() << " != 4)";
  EXPECT_EQ(category_model1->count(), 4) << "Category 1 result count not as expected (" << category_model1->count() << " != 4)";
  EXPECT_EQ(category_model2->count(), 4) << "Category 2 result count not as expected (" << category_model2->count() << " != 4)";
  EXPECT_EQ(category_model3->count(), 0) << "Category 3 result count not as expected (" << category_model3->count() << " != 0)";
}

TEST_F(TestScopeProxy, TestActivateUri)
{
  // Auto-connect on activate

  bool activated_return = false;
  auto func = [&activated_return] (LocalResult const& result, ScopeHandledType handled, glib::HintsMap const&, glib::Error const& error) {
    activated_return = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::HIDE_DASH);
  };

  LocalResult result; result.uri = "file:://test";
  scope_proxy_->Activate(result,
                          UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT,
                          glib::HintsMap(),
                          func,
                          nullptr);

  Utils::WaitUntilMSec(activated_return,
                       2000,
                       glib::String(g_strdup("Failed to activate")));
}

TEST_F(TestScopeProxy, TestPreview)
{
  // Auto-connect on preview

  bool prevew_returned = false;
  auto func = [&prevew_returned] (LocalResult const& result, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error) {
    prevew_returned = true;

    EXPECT_TRUE(error==false) << "Preview request returned with error: " << error.Message();
    EXPECT_EQ(handled, ScopeHandledType::SHOW_PREVIEW) << "Preview request did not return SHOW_PREVIEW";
    EXPECT_TRUE(hints.find("preview") != hints.end()) << "Preview request did not return correct hints";
  };

  LocalResult result; result.uri = "file:://test";
  scope_proxy_->Activate(result,
                          UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT,
                          glib::HintsMap(),
                          func,
                          nullptr);

  Utils::WaitUntilMSec(prevew_returned,
                       2000,
                       glib::String(g_strdup("Failed to preview")));
}


} // namespace dash
} // namespace unity
