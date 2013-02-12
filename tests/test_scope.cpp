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

  Scope::Ptr scope_;
};

TEST_F(TestScope, TestConnection)
{
  ASSERT_TRUE(scope_->connected);
}


} // namespace dash
} // namespace unity
