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

namespace
{

static const string scope_name = "com.canonical.Unity.Test";
static const string scope_path = "/com/canonical/unity/scope/testscope1";

class MockScopeProxy : public ScopeProxy
{
public:
  MockScopeProxy():ScopeProxy(scope_name, scope_path) {}
};

class TestScopeProxy : public ::testing::Test
{
public:
  TestScopeProxy()
  {
    scope_proxy_.reset(new MockScopeProxy());
    // scope_proxy_->CreateProxy();
  }
 
  void ConnectAndWait()
  {
    scope_proxy_->CreateProxy();
    Utils::WaitUntilMSec([this] { return scope_proxy_->connected == true; }, true, 2000);
  }

  ScopeProxyInterface::Ptr scope_proxy_;
};

TEST_F(TestScopeProxy, TestConnection)
{
  ConnectAndWait();

  ASSERT_TRUE(scope_proxy_->connected);
}

TEST_F(TestScopeProxy, TestSynchronization)
{
  ConnectAndWait();

  EXPECT_EQ(scope_proxy_->search_hint(), "Search Test Scope");
  EXPECT_TRUE(scope_proxy_->visible());
  EXPECT_TRUE(scope_proxy_->search_in_global());

  EXPECT_FALSE(scope_proxy_->channel().empty());
}

TEST_F(TestScopeProxy, TestFilterSync)
{
  ConnectAndWait();

  bool filter_changed = false;
  scope_proxy_->filters.changed.connect([&filter_changed] (Filters::Ptr const& filters) {
    filter_changed = true;
  });
  Filters::Ptr filters = scope_proxy_->filters();
  Utils::WaitUntilMSec([filters] { return filters->count() > 0; }, true, 3000);

  FilterAdaptor adaptor = filters->RowAtIndex(0);

  Filter::Ptr filter = filters->FilterAtIndex(0);
  EXPECT_TRUE(filter ? true : false);
  EXPECT_EQ(filter->renderer_name, "filter-radiooption");

  RadioOptionFilter::Ptr radio_option_filter = std::static_pointer_cast<RadioOptionFilter>(filter);
  RadioOptionFilter::RadioOptions radio_opitons = radio_option_filter->options();
  EXPECT_TRUE(radio_opitons.size() > 0);

  radio_opitons[0]->active = !radio_opitons[0]->active();
  
  // Utils::WaitUntilMSec([&filter_changed] { return filter_changed == true; }, true, 2000);
}

TEST_F(TestScopeProxy, TestCategoryModel)
{
  ConnectAndWait();

  Categories::Ptr categories = scope_proxy_->categories();
  Utils::WaitUntilMSec([categories] { return categories->count() > 0; }, true, 3000);
}

TEST_F(TestScopeProxy, TestSearch)
{
  ConnectAndWait();

  bool search_ok = false;
  bool search_finished = false;
  scope_proxy_->Search("12:test_search", [&search_ok, &search_finished] (glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  }, nullptr);

  Results::Ptr results = scope_proxy_->results();
  Utils::WaitUntilMSec([&, results] { return search_finished == true && results->count() > 0; }, true, 2000);
  EXPECT_TRUE(search_ok);
}

TEST_F(TestScopeProxy, TestSearchCategories)
{
  // Dont create the proxy. It should do it automatically.

  scope_proxy_->Search("12:test_search", [&] (glib::HintsMap const&, glib::Error const& error) { }, nullptr);

  Results::Ptr results = scope_proxy_->results();
  Utils::WaitUntilMSec([results] { return results->count() == 12; }, true, 3000);

  Results::Ptr category_model0 = scope_proxy_->GetResultsForCategory(0);
  Results::Ptr category_model1 = scope_proxy_->GetResultsForCategory(1);
  Results::Ptr category_model2 = scope_proxy_->GetResultsForCategory(2);
  Results::Ptr category_model3 = scope_proxy_->GetResultsForCategory(3);

  EXPECT_EQ(category_model0->count(), 4);
  EXPECT_EQ(category_model1->count(), 4);
  EXPECT_EQ(category_model2->count(), 4);
  EXPECT_EQ(category_model3->count(), 0);
}

TEST_F(TestScopeProxy, TestActivateUri)
{
  // Dont create the proxy. It should do it automatically.

  bool activated_return = false;
  auto func = [&activated_return] (std::string const& uri, ScopeHandledType handled, glib::HintsMap const&, glib::Error const& error) {
    activated_return = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::HIDE_DASH);
  };

  scope_proxy_->Activate("file:://test",
                          UNITY_PROTOCOL_ACTION_TYPE_ACTIVATE_RESULT,
                          glib::HintsMap(),
                          func,
                          nullptr);

  Utils::WaitUntilMSec([&activated_return] { return activated_return; }, true, 3000);
}

TEST_F(TestScopeProxy, TestPreview)
{
  // Dont create the proxy. It should do it automatically.

  bool activated_return = false;
  auto func = [&activated_return] (std::string const& uri, ScopeHandledType handled, glib::HintsMap const& hints, glib::Error const& error) {
    activated_return = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::SHOW_PREVIEW);
    EXPECT_TRUE(hints.find("preview") != hints.end());
  };

  scope_proxy_->Activate("file:://test",
                          UNITY_PROTOCOL_ACTION_TYPE_PREVIEW_RESULT,
                          glib::HintsMap(),
                          func,
                          nullptr);

  Utils::WaitUntilMSec([&activated_return] { return activated_return; }, true, 3000);
}


}
