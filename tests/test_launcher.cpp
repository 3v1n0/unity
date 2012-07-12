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

#include "DNDCollectionWindow.h"
#include "MockLauncherIcon.h"
#include "Launcher.h"
#include "test_utils.h"
using namespace unity;

namespace
{

class MockMockLauncherIcon : public launcher::MockLauncherIcon
{
public:
  typedef nux::ObjectPtr<MockMockLauncherIcon> Ptr;

  MOCK_METHOD1(ShouldHighlightOnDrag, bool(DndData const&));
};

class TestLauncher : public Test
{
public:
  TestLauncher()
    : parent_window_(new nux::BaseWindow("TestLauncherWindow"))
    , dnd_collection_window_(new DNDCollectionWindow)
    , model_(new launcher::LauncherModel)
    , launcher_(new launcher::Launcher(parent_window_, dnd_collection_window_))
  {
    launcher_->SetModel(model_);
  }

  nux::BaseWindow* parent_window_;
  nux::ObjectPtr<DNDCollectionWindow> dnd_collection_window_;
  launcher::LauncherModel::Ptr model_;
  nux::ObjectPtr<launcher::Launcher> launcher_;
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

  EXPECT_FALSE(first->GetQuirk(launcher::AbstractLauncherIcon::QUIRK_DESAT));
  EXPECT_FALSE(second->GetQuirk(launcher::AbstractLauncherIcon::QUIRK_DESAT));
  EXPECT_TRUE(third->GetQuirk(launcher::AbstractLauncherIcon::QUIRK_DESAT));
}

}
