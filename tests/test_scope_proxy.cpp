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
#include "test_mock_scope.h"
#include "RadioOptionFilter.h"

using namespace std;

namespace unity
{
namespace dash
{

namespace
{
static const string scope_name = "com.canonical.Unity.Test";
static const string scope_path = "/com/canonical/unity/scope/testscope1";
 
static void ConnectAndWait(ScopeProxyInterface::Ptr const proxy)
{
  proxy->ConnectProxy();
  Utils::WaitUntilMSec([proxy] { return proxy->connected == true; },
                       true,
                       3000,
                       [] { return g_strdup("Could not connect to proxy"); });
}

};

TEST(TestScopeProxy, Connection)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  ConnectAndWait(scope_proxy);

  EXPECT_EQ(scope_proxy->connected(), true);
}

TEST(TestScopeProxy, ConnectionFail)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("fail", "this.is.a.fail.test", "this/is/a/fail/test"))));
  scope_proxy->ConnectProxy();

  Utils::WaitForTimeoutMSec(1000);
  EXPECT_EQ(scope_proxy->connected, false);
}

TEST(TestScopeProxy, DisconnectReconnect)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  ConnectAndWait(scope_proxy);

  scope_proxy->DisconnectProxy();
  EXPECT_EQ(scope_proxy->connected(), false);

  ConnectAndWait(scope_proxy);
}

TEST(TestScopeProxy, SetViewType)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  ASSERT_EQ(scope_proxy->view_type(), dash::HIDDEN);
  ConnectAndWait(scope_proxy);

  bool scope_view_type_changed = false;
  scope_proxy->view_type.changed.connect([&scope_view_type_changed] (ScopeViewType const&) { scope_view_type_changed = true; });
  scope_proxy->view_type = dash::SCOPE_VIEW;

  EXPECT_TRUE(scope_view_type_changed);
}

TEST(TestScopeProxy, FilterSync)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  ConnectAndWait(scope_proxy);

  bool filter_changed = false;
  scope_proxy->filters.changed.connect([&filter_changed] (Filters::Ptr const& filters) { filter_changed = true; });
  Filters::Ptr filters = scope_proxy->filters();
  Utils::WaitUntilMSec([filters] { return filters->count() > 0; },
                       true,
                       3000,
                       [] { return g_strdup("Filters coutn = 0"); });

  FilterAdaptor adaptor = filters->RowAtIndex(0);

  Filter::Ptr filter = filters->FilterAtIndex(0);
  EXPECT_TRUE(filter ? true : false);
  EXPECT_EQ(filter->renderer_name(), "filter-checkoption");

  RadioOptionFilter::Ptr radio_option_filter = std::static_pointer_cast<RadioOptionFilter>(filter);
  RadioOptionFilter::RadioOptions radio_opitons = radio_option_filter->options();
  EXPECT_TRUE(radio_opitons.size() > 0);
}

TEST(TestScopeProxy, CategorySync)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  ConnectAndWait(scope_proxy);

  Categories::Ptr categories = scope_proxy->categories();
  Utils::WaitUntilMSec([categories] { return categories->count() > 0; },
                       true,
                       3000,
                       [] { return g_strdup("Cateogry count = 0"); });
}

TEST(TestScopeProxy, Search)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // Auto-connect on search

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (std::string const& search_string, glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    if (error)
      printf("Error: %s\n", error.Message().c_str());
    search_finished = true;
  };

  scope_proxy->Search("12:cat", glib::HintsMap(), search_callback, nullptr);

  Results::Ptr results = scope_proxy->results();
  Utils::WaitUntilMSec([&search_finished, results] { return search_finished == true && results->count() == 12; },
                       true,
                       3000,
                       [results] { return g_strdup_printf("Either search didn't finish, or result count is not as expected (%u != 12).", static_cast<unsigned>(results->count())); });
  EXPECT_EQ(search_ok, true);
}

// Test that searching twice will not concatenate results.
TEST(TestScopeProxy, MultiSearch)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // Auto-connect on search

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (std::string const& search_string, glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  };

  // First Search
  scope_proxy->Search("12:cat", glib::HintsMap(), search_callback, nullptr);

  Results::Ptr results = scope_proxy->results();
  Utils::WaitUntilMSec([&search_finished, results] { return search_finished == true && results->count() == 12; },
                       true,
                       3000,
                       [results] { return g_strdup_printf("First search. Either search didn't finish, or result count is not as expected (%u != 12).", static_cast<int>(results->count())); });
  EXPECT_EQ(search_ok, true);

  // Second Search
  search_ok = false;
  search_finished = false;
  scope_proxy->Search("5:cat", glib::HintsMap(), search_callback, nullptr);
  
  Utils::WaitUntilMSec([&search_finished, results] { return search_finished == true && results->count() == 5; },
                       true,
                       3000,
                       [results] { return g_strdup_printf("Second search. Either search didn't finish, or result count is not as expected (%u != 5).", static_cast<unsigned>(results->count())); });
  EXPECT_EQ(search_ok, true);
}

TEST(TestScopeProxy, SearchFail)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("fail", "this.is.a.fail.test", "this/is/a/fail/test"))));
  // Auto-connect on search

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (std::string const& search_string, glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  };

  scope_proxy->Search("12:cat", glib::HintsMap(), search_callback, nullptr);
  Utils::WaitUntilMSec(search_finished,
                       3000,
                       [] { return g_strdup("Search did not finish."); });
  EXPECT_EQ(search_ok, false);
}

TEST(TestScopeProxy, SearchCancelled)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // make sure we're connect first. "so we can test quicker"
  ConnectAndWait(scope_proxy);

  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (std::string const& search_string, glib::HintsMap const&, glib::Error const& error) {
    search_finished = true;
  };

  glib::Object<GCancellable> cancel_search(g_cancellable_new());
  scope_proxy->Search("12:cat", glib::HintsMap(), search_callback, cancel_search);
  g_cancellable_cancel(cancel_search);

  Utils::WaitForTimeoutMSec(1000);
  EXPECT_EQ(search_finished, false);
}

TEST(TestScopeProxy, SearchCategories)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // Auto-connect on search

  scope_proxy->Search("12:cat", glib::HintsMap(), nullptr, nullptr);

  Results::Ptr results = scope_proxy->results();
  Utils::WaitUntilMSec([results] { return results->count() == 12; },
                       true,
                       3000,
                       [results] { return g_strdup_printf("Result count is not as expected (%u != 12).", static_cast<unsigned>(results->count())); });

  Results::Ptr category_model0 = scope_proxy->GetResultsForCategory(0);
  ASSERT_TRUE(category_model0 ? true : false);
  Results::Ptr category_model1 = scope_proxy->GetResultsForCategory(1);
  ASSERT_TRUE(category_model1 ? true : false);
  Results::Ptr category_model2 = scope_proxy->GetResultsForCategory(2);
  ASSERT_TRUE(category_model2 ? true : false);
  Results::Ptr category_model3 = scope_proxy->GetResultsForCategory(3);
  ASSERT_TRUE(category_model3 ? true : false);

  EXPECT_EQ(category_model0->count(), 4) << "Category 0 result count not as expected (" << category_model0->count() << " != 4)";
  EXPECT_EQ(category_model1->count(), 4) << "Category 1 result count not as expected (" << category_model1->count() << " != 4)";
  EXPECT_EQ(category_model2->count(), 4) << "Category 2 result count not as expected (" << category_model2->count() << " != 4)";
  EXPECT_EQ(category_model3->count(), 0) << "Category 3 result count not as expected (" << category_model3->count() << " != 0)";
}

TEST(TestScopeProxy, ActivateUri)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // Auto-connect on search

  bool activated_return = false;
  auto activate_callback = [&activated_return] (LocalResult const& result, ScopeHandledType handled, glib::HintsMap const&, glib::Error const& error) {
    activated_return = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::HIDE_DASH);
  };

  LocalResult result; result.uri = "file:://test";
  scope_proxy->Activate(result,
                          UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT,
                          glib::HintsMap(),
                          activate_callback,
                          nullptr);

  Utils::WaitUntilMSec(activated_return,
                       3000,
                       [] { return g_strdup("Failed to activate"); });
}

TEST(TestScopeProxy, Preview)
{
  ScopeProxyInterface::Ptr scope_proxy(new ScopeProxy(ScopeData::Ptr(new MockScopeData("testscope1", scope_name, scope_path))));
  // Auto-connect on search

  bool prevew_returned = false;
  auto activate_callback = [&prevew_returned] (LocalResult const& result, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error) {
    prevew_returned = true;

    EXPECT_TRUE(error==false) << "Preview request returned with error: " << error.Message();
    EXPECT_EQ(handled, ScopeHandledType::SHOW_PREVIEW) << "Preview request did not return SHOW_PREVIEW";
    EXPECT_TRUE(hints.find("preview") != hints.end()) << "Preview request did not return correct hints";
  };

  LocalResult result; result.uri = "file:://test";
  scope_proxy->Activate(result,
                          UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT,
                          glib::HintsMap(),
                          activate_callback,
                          nullptr);

  Utils::WaitUntilMSec(prevew_returned,
                       3000,
                       [] { return g_strdup("Failed to preview"); });
}

} // namespace dash
} // namespace unity
