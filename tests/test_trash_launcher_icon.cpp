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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 */

#include <gmock/gmock.h>
#include <UnityCore/DesktopUtilities.h>

#include "TrashLauncherIcon.h"
#include "test_mock_filemanager.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;

namespace
{

struct TestTrashLauncherIcon : Test
{
  TestTrashLauncherIcon()
    : fm_(std::make_shared<NiceMock<MockFileManager>>())
    , icon(fm_)
  {}

  MockFileManager::Ptr fm_;
  TrashLauncherIcon icon;
};

TEST_F(TestTrashLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::END);
}

TEST_F(TestTrashLauncherIcon, Activate)
{
  unsigned long long time = g_random_int();
  EXPECT_CALL(*fm_, OpenTrash(time));
  icon.Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, time));
}

TEST_F(TestTrashLauncherIcon, Quicklist)
{
  auto const& menus = icon.Menus();
  EXPECT_EQ(menus.size(), 1);
}

TEST_F(TestTrashLauncherIcon, QuicklistEmptyTrash)
{
  auto const& menus = icon.Menus();
  ASSERT_EQ(menus.size(), 1);

  auto const& empty_trash_menu = menus.front();

  unsigned time = g_random_int();
  EXPECT_CALL(*fm_, EmptyTrash(time));
  dbusmenu_menuitem_handle_event(empty_trash_menu, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
}

TEST_F(TestTrashLauncherIcon, RunningState)
{
  EXPECT_CALL(*fm_, IsTrashOpened()).WillRepeatedly(Return(true));
  fm_->locations_changed.emit();
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));

  EXPECT_CALL(*fm_, IsTrashOpened()).WillRepeatedly(Return(false));
  fm_->locations_changed.emit();
  EXPECT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestTrashLauncherIcon, FilemanagerSignalDisconnection)
{
  auto file_manager = std::make_shared<NiceMock<MockFileManager>>();
  {
    TrashLauncherIcon trash_icon(file_manager);
    ASSERT_FALSE(file_manager->locations_changed.empty());
  }

  EXPECT_TRUE(file_manager->locations_changed.empty());
}

}
