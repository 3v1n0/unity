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
#include <gmock/gmock.h>

#include <Nux/Nux.h>
#include <NuxCore/ObjectPtr.h>

#include "dash/DashView.h"
#include "dash/ApplicationStarter.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

#include "test_mock_scope.h"
#include "test_utils.h"

using namespace unity;
using namespace unity::dash;
using namespace testing;

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

struct MockApplicationStarter : public unity::ApplicationStarter {
  typedef std::shared_ptr<MockApplicationStarter> Ptr;
  MOCK_METHOD2(Launch, bool(std::string const&, Time));
};

class TestDashView : public ::testing::Test
{
public:
  TestDashView()
  : application_starter_(std::make_shared<MockApplicationStarter>())
  {}

  virtual void SetUp() { Utils::init_gsettings_test_environment(); }
  virtual void TearDown() { Utils::reset_gsettings_test_environment(); }

  class MockDashView  : public DashView
  {
  public:
    MockDashView(Scopes::Ptr const& scopes, ApplicationStarter::Ptr const& application_starter)
    : DashView(scopes, application_starter)
    {
    }

    using DashView::scope_views_;
  };

protected:
  Settings unity_settings_;
  dash::Style dash_style_;
  panel::Style panel_style_;
  MockApplicationStarter::Ptr application_starter_;
};

TEST_F(TestDashView, TestConstruct)
{
  Scopes::Ptr scopes(new MockGSettingsScopes(scopes_default));
  nux::ObjectPtr<MockDashView> view(new MockDashView(scopes, application_starter_));

  EXPECT_EQ(view->scope_views_.size(), 4) << "Error: Incorrect number of scope views (" << view->scope_views_.size() << " != 4)";
}


TEST_F(TestDashView, LensActivatedSignal)
{
  Scopes::Ptr scopes(new MockGSettingsScopes(scopes_default));
  nux::ObjectPtr<MockDashView> view(new MockDashView(scopes, application_starter_));

  LocalResult result;
  result.uri = "application://uri";

  EXPECT_CALL(*application_starter_, Launch("uri", _)).Times(1);
  scopes->GetScopeAtIndex(0)->activated.emit(result, NOT_HANDLED, glib::HintsMap());

  EXPECT_CALL(*application_starter_, Launch("uri", _)).Times(1);
  scopes->GetScopeAtIndex(0)->activated.emit(result, NOT_HANDLED, glib::HintsMap());
}

}
}
