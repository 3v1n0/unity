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
 * Authored by: Andrea Azzarone <andrea.azzarone@canonical.com>
 */

#include <gmock/gmock.h>

#include <UnityCore/Lenses.h>

#include "ApplicationStarter.h"
#include "DashView.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"

#include "mock-lenses.h"

using namespace unity::dash;
using namespace testing;

namespace  {

struct MockApplicationStarter : public unity::ApplicationStarter {
  typedef std::shared_ptr<MockApplicationStarter> Ptr;
  MOCK_METHOD2(Launch, bool(std::string const&, Time));
};


struct TestDashView : public testing::Test {
  TestDashView()
    : lenses_(std::make_shared<testmocks::ThreeMockTestLenses>())
    , application_starter_(std::make_shared<MockApplicationStarter>())
    , dash_view_(new DashView(lenses_, application_starter_))
  {}

  unity::Settings unity_settings_;
  Style dash_style;
  unity::panel::Style panel_style;
  Lenses::Ptr lenses_;
  MockApplicationStarter::Ptr application_starter_;
  nux::ObjectPtr<DashView> dash_view_;
};


TEST_F(TestDashView, LensActivatedSignal)
{
  EXPECT_CALL(*application_starter_, Launch("uri", _)).Times(1);
  lenses_->GetLensAtIndex(0)->activated.emit("0xaabbcc:application://uri", NOT_HANDLED, Lens::Hints());

  EXPECT_CALL(*application_starter_, Launch("uri", _)).Times(1);
  lenses_->GetLensAtIndex(0)->activated.emit("0xaabbcc:unity-runner://uri", NOT_HANDLED, Lens::Hints());
}

}
