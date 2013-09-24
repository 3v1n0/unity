/*
 * Copyright 2012-2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 *              Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
using namespace testing;

#include "shortcuts/BaseWindowRaiser.h"
#include "shortcuts/ShortcutController.h"
#include "unity-shared/UnitySettings.h"
#include "UBusMessages.h"
using namespace unity;

#include "test_utils.h"
#include "test_uscreen_mock.h"

#include <NuxCore/AnimationController.h>

namespace
{

struct MockBaseWindowRaiser : public shortcut::BaseWindowRaiser
{
  typedef std::shared_ptr<MockBaseWindowRaiser> Ptr;

  MOCK_METHOD1 (Raise, void(nux::ObjectPtr<nux::BaseWindow> window));
};

struct StandaloneModeller : shortcut::AbstractModeller
{
  shortcut::Model::Ptr GetCurrentModel() const
  {
    return std::make_shared<shortcut::Model>(std::list<shortcut::AbstractHint::Ptr>());
  }
};
}

namespace unity
{
namespace shortcut
{
class TestShortcutController : public Test
{
  struct MockShortcutController : public Controller
  {
    MockShortcutController(BaseWindowRaiser::Ptr const& base_window_raiser,
                           AbstractModeller::Ptr const& modeller)
      : Controller(base_window_raiser, modeller)
    {}

    MOCK_METHOD1(SetOpacity, void(double));
    using Controller::GetOffsetPerMonitor;
    using Controller::ConstructView;
    using Controller::view_;

    void RealSetOpacity(double value)
    {
      Controller::SetOpacity(value);
    }
  };

public:
  TestShortcutController()
    : base_window_raiser_(std::make_shared<NiceMock<MockBaseWindowRaiser>>())
    , modeller_(std::make_shared<StandaloneModeller>())
    , controller_(base_window_raiser_, modeller_)
    , animation_controller_(tick_source_)
  {
    ON_CALL(controller_, SetOpacity(_))
      .WillByDefault(Invoke(&controller_, &MockShortcutController::RealSetOpacity));
  }

  MockUScreen uscreen;
  Settings unity_settings;
  MockBaseWindowRaiser::Ptr base_window_raiser_;
  AbstractModeller::Ptr modeller_;
  NiceMock<MockShortcutController> controller_;

  nux::animation::TickSource tick_source_;
  nux::animation::AnimationController animation_controller_;
};

TEST_F(TestShortcutController, WindowIsRaisedOnShow)
{
  EXPECT_CALL(*base_window_raiser_, Raise(_))
    .Times(1);

  controller_.Show();
  Utils::WaitForTimeout(1);
}

TEST_F(TestShortcutController, HiddeenOnConstruction)
{
  {
    InSequence sequence;
    EXPECT_CALL(controller_, SetOpacity(0.0))
      .Times(1);
    EXPECT_CALL(controller_, SetOpacity(_))
      .Times(0);
  }

  controller_.ConstructView();
  controller_.Hide();
}

TEST_F(TestShortcutController, GetGeometryPerMonitor)
{
  nux::Geometry good_monitor(0, 0, 1366, 768);
  nux::Geometry small_monitor(good_monitor.x + good_monitor.width, 0, 1, 1);
  uscreen.SetMonitors({good_monitor, small_monitor});

  nux::Point offset(g_random_int_range(0, 10), g_random_int_range(0, 10));
  controller_.SetAdjustment(offset.x, offset.y);
  controller_.ConstructView();

  // Big enough monitor
  nux::Point expected(good_monitor.x + offset.x, good_monitor.y + offset.y);
  auto const& view_geo = controller_.view_->GetAbsoluteGeometry();
  expected.x += (good_monitor.width - view_geo.width - offset.x) / 2;
  expected.y += (good_monitor.height - view_geo.height - offset.y) / 2;
  EXPECT_EQ(controller_.GetOffsetPerMonitor(0), expected);

  // Too small monitor
  EXPECT_LT(controller_.GetOffsetPerMonitor(1).x, 0);
  EXPECT_LT(controller_.GetOffsetPerMonitor(1).y, 0);
}

TEST_F(TestShortcutController, ModelIsChangedOnModellerChange)
{
  controller_.ConstructView();
  auto old_model = controller_.view_->GetModel();
  auto model = std::make_shared<Model>(std::list<AbstractHint::Ptr>());
  modeller_->model_changed(model);

  ASSERT_NE(controller_.view_->GetModel(), old_model);
  EXPECT_EQ(controller_.view_->GetModel(), model);
}

TEST_F(TestShortcutController, DisabledOnLauncherKeySwitcherStart)
{
  ASSERT_TRUE(controller_.IsEnabled());
  UBusManager().SendMessage(UBUS_LAUNCHER_START_KEY_SWITCHER);

  Utils::WaitUntilMSec([this] { return controller_.IsEnabled(); }, false);
}

TEST_F(TestShortcutController, EnabledOnLauncherKeySwitcherEnd)
{
  controller_.SetEnabled(false);
  ASSERT_FALSE(controller_.IsEnabled());
  UBusManager().SendMessage(UBUS_LAUNCHER_END_KEY_SWITCHER);

  Utils::WaitUntilMSec([this] { return controller_.IsEnabled(); }, true);
}

TEST_F(TestShortcutController, HideOnDashShow)
{
  controller_.Show();
  ASSERT_TRUE(controller_.Visible());
  UBusManager().SendMessage(UBUS_OVERLAY_SHOWN);

  Utils::WaitUntilMSec([this] { return controller_.Visible(); }, false);
}

}
}
