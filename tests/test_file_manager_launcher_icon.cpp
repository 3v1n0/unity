/*
 * Copyright 2016 Canonical Ltd.
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

#include "FileManagerLauncherIcon.h"
#include "UnityCore/DesktopUtilities.h"

#include "test_mock_devices.h"
#include "test_mock_filemanager.h"
#include "mock-application.h"

using namespace unity;
using namespace unity::launcher;
using namespace testmocks;

namespace
{

const std::string TRASH_URI = "trash:";
const std::string TRASH_PATH = "file://" + DesktopUtilities::GetUserTrashDirectory();

struct TestFileManagerLauncherIcon : public Test
{
  TestFileManagerLauncherIcon()
    : app_(std::make_shared<MockApplication::Nice>())
    , fm_(std::make_shared<MockFileManager::Nice>())
    , dev_ (std::make_shared<MockDeviceLauncherSection>(2))
    , icon_(new FileManagerLauncherIcon(app_, dev_, fm_))
  {
  }

  MockApplication::Ptr app_;
  MockFileManager::Ptr fm_;
  DeviceLauncherSection::Ptr dev_;
  FileManagerLauncherIcon::Ptr icon_;
};

TEST_F(TestFileManagerLauncherIcon, IconType)
{
  EXPECT_EQ(icon_->GetIconType(), AbstractLauncherIcon::IconType::APPLICATION);
}

TEST_F(TestFileManagerLauncherIcon, NoWindow)
{
  EXPECT_FALSE(icon_->IsVisible());
}

TEST_F(TestFileManagerLauncherIcon, NoManagedWindow_TrashUri)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(TRASH_URI));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_FALSE(icon_->IsVisible());
  EXPECT_FALSE(icon_->IsRunning());
}

TEST_F(TestFileManagerLauncherIcon, NoManagedWindow_TrashPath)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(TRASH_PATH));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_FALSE(icon_->IsVisible());
  EXPECT_FALSE(icon_->IsRunning());
}

TEST_F(TestFileManagerLauncherIcon, NoManagedWindow_Device)
{
  auto const& device_icons = dev_->GetIcons();
  ASSERT_EQ(2u, device_icons.size());

  device_icons.at(0)->Activate(ActionArg());

  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(device_icons.at(0)->GetVolumeUri()));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_FALSE(icon_->IsVisible());
  EXPECT_FALSE(icon_->IsRunning());
}

TEST_F(TestFileManagerLauncherIcon, ManagedWindows)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return("/usr/bin"));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(icon_->IsRunning());
}

TEST_F(TestFileManagerLauncherIcon, ManagedWindows_EmptyLocation)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(""));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(icon_->IsRunning());
}

TEST_F(TestFileManagerLauncherIcon, ManagedWindows_CopyDialog)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(1);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(""));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  win->visible_ = false;
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(icon_->IsRunning());
}


TEST_F(TestFileManagerLauncherIcon, ManagedWindows_CopyDialogAndManagedWindow)
{
  EXPECT_CALL(*fm_, LocationForWindow(_)).Times(3);
  ON_CALL(*fm_, LocationForWindow(_)).WillByDefault(Return(""));

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  win->visible_ = false;
  app_->windows_.push_back(win);
  app_->window_opened.emit(win);

  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(icon_->IsRunning());
  EXPECT_EQ(2u, icon_->WindowsVisibleOnMonitor(0));
}

TEST_F(TestFileManagerLauncherIcon, ManagedWindows_CopyDialogAndNoManagedWindow)
{
  {
    InSequence s;

    EXPECT_CALL(*fm_, LocationForWindow(_))
      .Times(1)
      .WillOnce(Return(""));

    EXPECT_CALL(*fm_, LocationForWindow(_))
      .Times(1)
      .WillOnce(Return(""));

    EXPECT_CALL(*fm_, LocationForWindow(_))
      .Times(1)
      .WillOnce(Return(TRASH_PATH));
  }

  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  app_->windows_ = { win };
  app_->window_opened.emit(win);

  win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  win->visible_ = false;
  app_->windows_.push_back(win);
  app_->window_opened.emit(win);

  EXPECT_TRUE(icon_->IsVisible());
  EXPECT_TRUE(icon_->IsRunning());
  EXPECT_EQ(1u, icon_->WindowsVisibleOnMonitor(0));
}

}
