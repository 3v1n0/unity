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
#include <Nux/NuxTimerTickSource.h>
#include <NuxCore/AnimationController.h>
#include "HudController.h"
#include "mock-base-window.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/WindowManager.h"
#include "test_utils.h"
using namespace unity;

namespace
{

const unsigned ANIMATION_DURATION = 90 * 1000; // in microseconds
const unsigned TICK_DURATION = 10 * 1000;

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
  MOCK_METHOD1(AddProperties, void(debug::IntrospectionData&));
  MOCK_METHOD2(Draw, void(nux::GraphicsEngine&, bool));
  nux::Geometry GetContentGeometry()
  {
    return nux::Geometry();
  }
};


struct TestHudController : Test
{
  TestHudController()
    : view_(new NiceMock<MockHudView>)
    , base_window_(new testmocks::MockBaseWindow::Nice())
    , controller_(std::make_shared<hud::Controller>([this] { return view_.GetPointer(); },
                                                    [this] { return base_window_.GetPointer(); }))
  {}

protected:
  // required to create hidden secret global variables
  Settings unity_settings_;
  dash::Style dash_style_;
  panel::Style panel_style_;

  MockHudView::Ptr view_;
  testmocks::MockBaseWindow::Ptr base_window_;
  hud::Controller::Ptr controller_;
};

TEST_F(TestHudController, Construction)
{
  EXPECT_CALL(*base_window_, SetOpacity(0.0f));
  controller_ = std::make_shared<hud::Controller>([this] { return view_.GetPointer(); },
                                                  [this] { return base_window_.GetPointer(); });
}

TEST_F(TestHudController, TestShowAndHideHud)
{
  long long t;
  long long global_tick = 0;
  nux::NuxTimerTickSource tick_source;
  nux::animation::AnimationController animation_controller(tick_source);

  // Verify initial conditions
  EXPECT_EQ(base_window_->GetOpacity(), 0.0);

  // Set expectations for showing the HUD
  EXPECT_CALL(*view_, AboutToShow()).Times(1);
  EXPECT_CALL(*view_, ResetToDefault()).Times(1);
  {
    InSequence showing;
    EXPECT_CALL(*base_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));
    EXPECT_CALL(*base_window_, SetOpacity(AllOf(Gt(0.0f), Lt(1.0f))))
      .Times(AtLeast(ANIMATION_DURATION/TICK_DURATION-1));
    EXPECT_CALL(*base_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));
  }

  controller_->ShowHud();
  for (t = global_tick; t < global_tick + ANIMATION_DURATION+1; t += TICK_DURATION)
    tick_source.tick(t);
  global_tick += t;

  EXPECT_EQ(base_window_->GetOpacity(), 1.0);

  Mock::VerifyAndClearExpectations(view_.GetPointer());
  Mock::VerifyAndClearExpectations(base_window_.GetPointer());

  // Set expectations for hiding the HUD
  EXPECT_CALL(*view_, AboutToHide()).Times(1);
  {
    InSequence hiding;
    EXPECT_CALL(*base_window_, SetOpacity(Eq(1.0f))).Times(AtLeast(1));
    EXPECT_CALL(*base_window_, SetOpacity(AllOf(Lt(1.0f), Gt(0.0f))))
      .Times(AtLeast(ANIMATION_DURATION/TICK_DURATION-1));
    EXPECT_CALL(*base_window_, SetOpacity(Eq(0.0f))).Times(AtLeast(1));
  }

  controller_->HideHud();
  for (t = global_tick; t < global_tick + ANIMATION_DURATION+1; t += TICK_DURATION)
    tick_source.tick(t);
  global_tick += t;

  EXPECT_EQ(base_window_->GetOpacity(), 0.0);
}

TEST_F(TestHudController, DisconnectWMSignalsOnDestruction)
{
  auto& signal = WindowManager::Default().initiate_spread;
  size_t before = signal.size();
  {
    auto controller = std::make_shared<hud::Controller>([this] { return view_.GetPointer(); },
                                                        [this] { return base_window_.GetPointer(); });
  }
  ASSERT_EQ(before, signal.size());
  signal.emit();
}

}
