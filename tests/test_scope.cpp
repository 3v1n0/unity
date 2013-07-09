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
    Utils::WaitUntil([this] { return scope_->connected() == true; }, true, 2);
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
                         true, 2000, "Filter '"+filter_to_wait_for+"' not found");
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
  auto search_callback = [&search_ok] (std::string const& search_string, glib::HintsMap const&, glib::Error const&) {
    search_ok = true;
  };

  scope_->Search("12:test_search", search_callback, nullptr);
  Utils::WaitUntil(search_ok, 2, "Search did not finish sucessfully");
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

  Utils::WaitUntil(activated_return, 2, "Failed to activate");
}

TEST_F(TestScope, PreviewPerformAction)
{
  Preview::Ptr preview;
  // Auto-connect on preview
  bool preview_ok = false;
  auto preview_callback = [&preview_ok, &preview] (LocalResult const&, Preview::Ptr const& _preview, glib::Error const&) {
    preview_ok = true;
    preview = _preview;
  };

  LocalResult result; result.uri = "file:://test";
  scope_->Preview(result,
                  preview_callback);

  Utils::WaitUntil(preview_ok, 2, "Failed to preview");
  EXPECT_TRUE(preview ? true : false);
  if (preview)
  {
    Preview::ActionPtrList actions = preview->GetActions();
    // EXPECT_TRUE(actions.size() > 0);
    for (auto action : actions)
      preview->PerformAction(action->id);    
  }
}

TEST_F(TestScope, ActivatePreviewAction)
{
  // Auto-connect on preview
  bool preview_action_ok = false;
  auto preview_action_callback = [&preview_action_ok] (LocalResult const&, ScopeHandledType, glib::Error const&) {
    preview_action_ok = true;
  };

  LocalResult result; result.uri = "file:://test";
  Preview::ActionPtr preview_action(new Preview::Action);
  preview_action->id = "action1";
  scope_->ActivatePreviewAction(preview_action,
                                result,
                                glib::HintsMap(),
                                preview_action_callback);

  Utils::WaitUntil(preview_action_ok, 2, "Failed to activate preview action");
}

TEST_F(TestScope, ActivatePreviewActionActivationUri)
{
  // Auto-connect on preview
  bool preview_action_ok = false;
  auto preview_action_callback = [&preview_action_ok] (LocalResult const&, ScopeHandledType, glib::Error const&) {
    preview_action_ok = true;
  };

  std::string uri_activated;
  scope_->activated.connect([&uri_activated] (LocalResult const& result, ScopeHandledType, glib::HintsMap const&) {
    uri_activated = result.uri;
  });

  LocalResult result; result.uri = "file:://test";
  Preview::ActionPtr preview_action(new Preview::Action);
  preview_action->id = "action1";
  preview_action->activation_uri = "uri://activation_uri";
  scope_->ActivatePreviewAction(preview_action,
                                result,
                                glib::HintsMap(),
                                preview_action_callback);

  Utils::WaitUntil(preview_action_ok, 2, "Failed to activate preview action");
  Utils::WaitUntil([&uri_activated] () { return uri_activated == "uri://activation_uri"; },
                   true, 2, "Activation signal not emitted from scope.");
}


TEST_F(TestScope, UpdateSearchCategoryWorkflow)
{
  bool search_ok = false;
  bool search_finished = false;
  auto search_callback = [&search_ok, &search_finished] (std::string const& search_string, glib::HintsMap const&, glib::Error const& error) {
    search_finished = true;
    search_ok = error ? false : true;
  };

  // 1. First search
  scope_->Search("13:cat", search_callback);

  Results::Ptr results = scope_->results();
  Utils::WaitUntil(search_ok, 2, "First search failed.");
  Utils::WaitUntil([results] { return results->count() == 13; }, true, 2,
                   "First search. Either search didn't finish, or result count " \
                   "is not as expected ("+std::to_string(results->count())+" != 13).");
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
  Utils::WaitUntil([results] { return results->count() == 4; }, true, 30,
                   "Category activate. Result count is not as expected ("+
                    std::to_string(results->count())+" != 4).");

  category_model0 = scope_->GetResultsForCategory(0);
  category_model1 = scope_->GetResultsForCategory(1);
  category_model2 = scope_->GetResultsForCategory(2);

  EXPECT_EQ(category_model0->count(), 0) << "Category 0 result count not as expected (" << category_model0->count() << " != 0)";
  EXPECT_EQ(category_model1->count(), 4) << "Category 1 result count not as expected (" << category_model1->count() << " != 4)";
  EXPECT_EQ(category_model2->count(), 0) << "Category 2 result count not as expected (" << category_model2->count() << " != 0)";
}

} // namespace dash
} // namespace unity
