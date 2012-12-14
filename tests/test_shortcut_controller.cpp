/*
 * Copyright 2012 Canonical Ltd.
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
 */

#include <gmock/gmock.h>
using namespace testing;

#include "shortcuts/BaseWindowRaiser.h"
#include "shortcuts/ShortcutController.h"
#include "unity-shared/UnitySettings.h"
using namespace unity;

#include "test_utils.h"

#include <NuxCore/AnimationController.h>

namespace
{

struct MockBaseWindowRaiser : public shortcut::BaseWindowRaiser
{
  typedef std::shared_ptr<MockBaseWindowRaiser> Ptr;

  MOCK_METHOD1 (Raise, void(nux::ObjectPtr<nux::BaseWindow> window));
};

struct MockShortcutController : public shortcut::Controller
{
  MockShortcutController(std::list<shortcut::AbstractHint::Ptr> const& hints,
                         shortcut::BaseWindowRaiser::Ptr const& base_window_raiser)
    : Controller(hints, base_window_raiser)
  {}

  MOCK_METHOD1(SetOpacity, void(double));

  void RealSetOpacity(double value)
  {
    Controller::SetOpacity(value);
  }
};

class TestShortcutController : public Test
{
public:
  TestShortcutController()
    : base_window_raiser_(std::make_shared<MockBaseWindowRaiser>())
    , controller_(hints_, base_window_raiser_)
    , animation_controller_(tick_source_)
  {
    ON_CALL(controller_, SetOpacity(_))
      .WillByDefault(Invoke(&controller_, &MockShortcutController::RealSetOpacity));
  }

  Settings unity_settings;
  std::list<shortcut::AbstractHint::Ptr> hints_;
  MockBaseWindowRaiser::Ptr base_window_raiser_;
  MockShortcutController controller_;

  nux::animation::TickSource tick_source_;
  nux::animation::AnimationController animation_controller_;
};

TEST_F (TestShortcutController, WindowIsRaisedOnShow)
{
  EXPECT_CALL(*base_window_raiser_, Raise(_))
    .Times(1);

  controller_.Show();
  Utils::WaitForTimeout(1);
}


TEST_F (TestShortcutController, Hide)
{
  {
    InSequence sequence;
    EXPECT_CALL(controller_, SetOpacity(0.0))
      .Times(1);
    EXPECT_CALL(controller_, SetOpacity(_))
      .Times(0);
  }

  controller_.Show();

  controller_.Hide();
  tick_source_.tick(1000);
}


}
