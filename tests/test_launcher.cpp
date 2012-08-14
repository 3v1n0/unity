// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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

#include <list>
#include <gmock/gmock.h>
using namespace testing;

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "launcher/DNDCollectionWindow.h"
#include "launcher/MockLauncherIcon.h"
#include "launcher/Launcher.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/UnitySettings.h"
#include "test_utils.h"
using namespace unity;

namespace unity
{
namespace launcher
{
namespace
{

const int STARTING_ANIMATION_DURATION = 150;

class MockMockLauncherIcon : public launcher::MockLauncherIcon
{
public:
  typedef nux::ObjectPtr<MockMockLauncherIcon> Ptr;

  MOCK_METHOD1(ShouldHighlightOnDrag, bool(DndData const&));
};

}

class TestLauncher : public Test
{
public:
  TestLauncher()
    : parent_window_(new nux::BaseWindow("TestLauncherWindow"))
    , dnd_collection_window_(new DNDCollectionWindow)
    , model_(new LauncherModel)
    , options_(new Options)
    , launcher_(new Launcher(parent_window_, dnd_collection_window_))
  {
    launcher_->options = options_;
    launcher_->SetModel(model_);
  }

  float IconBackgroundIntensity(AbstractLauncherIcon::Ptr icon, timespec const& current) const
  {
    return launcher_->IconBackgroundIntensity(icon, current);
  }

  nux::BaseWindow* parent_window_;
  nux::ObjectPtr<DNDCollectionWindow> dnd_collection_window_;
  Settings settings;
  panel::Style panel_style;
  LauncherModel::Ptr model_;
  Options::Ptr options_;
  nux::ObjectPtr<Launcher> launcher_;
};

TEST_F(TestLauncher, TestQuirksDuringDnd)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon);
  model_->AddIcon(third);

  EXPECT_CALL(*first, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*second, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*third, ShouldHighlightOnDrag(_))
      .WillRepeatedly(Return(false));

  std::list<char*> uris;
  dnd_collection_window_->collected.emit(uris);

  Utils::WaitForTimeout(1);

  EXPECT_FALSE(first->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_FALSE(second->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
  EXPECT_TRUE(third->GetQuirk(launcher::AbstractLauncherIcon::Quirk::DESAT));
}


TEST_F(TestLauncher, TestIconBackgroundIntensity)
{
  MockMockLauncherIcon::Ptr first(new MockMockLauncherIcon);
  model_->AddIcon(first);

  MockMockLauncherIcon::Ptr second(new MockMockLauncherIcon);
  model_->AddIcon(second);

  MockMockLauncherIcon::Ptr third(new MockMockLauncherIcon);
  model_->AddIcon(third);

  options_->backlight_mode = BACKLIGHT_NORMAL;
  options_->launch_animation = LAUNCH_ANIMATION_PULSE;

  first->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  second->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, true);
  third->SetQuirk(AbstractLauncherIcon::Quirk::RUNNING, false);

  Utils::WaitForTimeoutMSec(STARTING_ANIMATION_DURATION);
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  EXPECT_THAT(IconBackgroundIntensity(first, current), Gt(0.0f));
  EXPECT_THAT(IconBackgroundIntensity(second, current), Gt(0.0f));
  EXPECT_EQ(IconBackgroundIntensity(third, current), 0.0f);
}

}
}

