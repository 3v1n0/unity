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

#include "test_utils.h"
#include "RadioOptionFilter.h"

using namespace std;
using namespace unity;
using namespace unity::dash;

namespace
{
const std::string SCOPE_NAME = "testscope1.scope";

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

  Scope::Ptr scope_;
};

TEST_F(TestScope, TestConnection)
{
  ASSERT_TRUE(scope_->connected);
}

TEST_F(TestScope, TestSynchronization)
{
  EXPECT_EQ(scope_->search_hint(), "Search Test Scope");
  EXPECT_TRUE(scope_->visible());
  EXPECT_TRUE(scope_->search_in_global());
}

TEST_F(TestScope, TestSearch)
{
  bool search_ok = false;
  bool search_finished = false;
  scope_->Search("12:test_search", [&] (glib::HintsMap const&, glib::Error const& error) {
    search_ok = error ? false : true;
    search_finished = true;
  });

  Results::Ptr results = scope_->results();
  Utils::WaitUntilMSec([&, results] { return search_finished == true && results->count() > 0; }, true, 2000);
  EXPECT_TRUE(search_ok == true);
}

TEST_F(TestScope, TestActivateUri)
{
  bool activated_return = false;
  auto func = [&] (std::string const& uri, ScopeHandledType handled, glib::Error const& error) {
    activated_return = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::HIDE_DASH);
  };

  scope_->Activate("file:://test", func);

  Utils::WaitUntilMSec([&activated_return] { return activated_return; }, true, 2000);
}

TEST_F(TestScope, TestPreview)
{
  bool prevew_returned = false;
  auto func = [&] (std::string const& uri, ScopeHandledType handled, glib::Error const& error) {
    prevew_returned = true;

    EXPECT_TRUE(error==false);
    EXPECT_EQ(handled, ScopeHandledType::SHOW_PREVIEW);
  };
  scope_->Preview("file:://test", func);

  Utils::WaitUntilMSec([&prevew_returned] { return prevew_returned; }, true, 2000);
}

}
