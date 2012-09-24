/*
 * Copyright 2012 Canonical Ltd.
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
 * Authored by: Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <gmock/gmock.h>
using namespace testing;

#include "HudController.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "test_utils.h"
using namespace unity;

namespace
{

class MockHudView : public hud::AbstractView
{
public:
  typedef nux::ObjectPtr<MockHudView> Ptr;

  MOCK_METHOD0(AboutToShow, void());
  MOCK_METHOD0(AboutToHide, void());
  MOCK_METHOD0(Relayout, void());
  MOCK_METHOD0(ResetToDefault, void());
  MOCK_METHOD0(SearchFinished, void());
  MOCK_METHOD4(SetIcon, void(std::string const&, unsigned int tile_size, unsigned int size, unsigned int padding));
  MOCK_METHOD1(SetQueries, void(hud::Hud::Queries queries));
  MOCK_METHOD2(SetMonitorOffset, void(int x, int y));
  MOCK_METHOD1(ShowEmbeddedIcon, void(bool show));
  MOCK_CONST_METHOD0(default_focus, nux::View*());
  MOCK_CONST_METHOD0(GetName, std::string());
  MOCK_METHOD1(AddProperties, void(GVariantBuilder*));
  MOCK_METHOD2(Draw, void(nux::GraphicsEngine&, bool));
  nux::Geometry GetContentGeometry()
  {
    return nux::Geometry();
  }


};

class TestHudController : public Test
{
public:
  virtual void SetUp()
  {
    view = new MockHudView;
    controller.reset(new hud::Controller([this]{ return view.GetPointer(); }));
  }

  Settings unity_settings;
  dash::Style dash_style;
  panel::Style panel_style;

  hud::Controller::Ptr controller;
  MockHudView::Ptr view;
};

TEST_F(TestHudController, TestHideHud)
{
  controller->ShowHud();
  Utils::WaitForTimeout(1);

  EXPECT_CALL(*view, ResetToDefault())
    .Times(1);

  controller->HideHud();
  // view->ResetToDefault should be called at the end of the fade out effect. So wait for it.
  Utils::WaitForTimeout(2);
}

}
