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

namespace
{

class MockBaseWindowRaiser : public shortcut::BaseWindowRaiser
{
public:
  typedef std::shared_ptr<MockBaseWindowRaiser> Ptr;

  MOCK_METHOD1 (Raise, void(nux::ObjectPtr<nux::BaseWindow> window));
};

class TestShortcutController : public Test
{
public:
  TestShortcutController()
    : base_window_raiser_(std::make_shared<MockBaseWindowRaiser>())
    , controller_(hints_, base_window_raiser_)
  {}

  Settings unity_settings;
  std::list<shortcut::AbstractHint::Ptr> hints_;
  MockBaseWindowRaiser::Ptr base_window_raiser_;
  shortcut::Controller controller_;
};

TEST_F (TestShortcutController, WindowIsRaisedOnShow)
{
  EXPECT_CALL(*base_window_raiser_, Raise(_))
    .Times(1);

  controller_.Show();
  Utils::WaitForTimeout(1);
}

}
