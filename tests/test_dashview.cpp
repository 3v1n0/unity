/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 *
 */

#include <list>

#include <gtest/gtest.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>

#include "dash/DashView.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

#include "test_mock_scope.h"
#include "test_utils.h"

using namespace unity;
using namespace unity::dash;

namespace unity
{
namespace dash
{

namespace
{
const char* scopes_default[] =  { "testscope1.scope",
                                  "testscope2.scope",
                                  "testscope3.scope",
                                  "testscope4.scope",
                                  NULL };
}

class TestDashView : public ::testing::Test
{
public:
  TestDashView() {}

  virtual void SetUp() { Utils::init_gsettings_test_environment(); }
  virtual void TearDown() { Utils::reset_gsettings_test_environment(); }

  class MockDashView  : public DashView
  {
  public:
    MockDashView(ScopesCreator scopes_creator = nullptr)
    : DashView(scopes_creator)
    {
    }

    using DashView::scope_views_;
  };

private:
  Settings unity_settings_;
  dash::Style dash_style_;
  panel::Style panel_style_;
};

TEST_F(TestDashView, TestConstruct)
{
  auto scope_creator = [] () { return Scopes::Ptr(new MockGSettingsScopes(scopes_default)); };
  nux::ObjectPtr<MockDashView> view(new MockDashView(scope_creator));

  EXPECT_EQ(view->scope_views_.size(), 4) << "Error: Incorrect number of scope views (" << view->scope_views_.size() << " != 4)";
}


}
}
