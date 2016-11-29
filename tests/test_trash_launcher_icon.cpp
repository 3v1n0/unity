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
#include "mock-application.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;
using namespace testmocks;

namespace
{
const std::string TRASH_URI = "trash:";

struct TestTrashLauncherIcon : testmocks::TestUnityAppBase
{
  TestTrashLauncherIcon()
    : fm_(std::make_shared<NiceMock<MockFileManager>>())
    , icon(fm_)
  {}

  MockFileManager::Ptr fm_;
  TrashLauncherIcon icon;
};

TEST_F(TestTrashLauncherIcon, InitState)
{
  EXPECT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
  EXPECT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));
}

TEST_F(TestTrashLauncherIcon, Position)
{
  EXPECT_EQ(icon.position(), AbstractLauncherIcon::Position::END);
}

TEST_F(TestTrashLauncherIcon, Activate)
{
  uint64_t time = g_random_int();
  EXPECT_CALL(*fm_, OpenTrash(time));
  icon.Activate(ActionArg(ActionArg::Source::LAUNCHER, 0, time));
}

TEST_F(TestTrashLauncherIcon, Quicklist)
{
  auto const& menus = icon.Menus();
  EXPECT_EQ(menus.size(), 1u);
}

TEST_F(TestTrashLauncherIcon, QuicklistEmptyTrash)
{
  auto const& menus = icon.Menus();
  ASSERT_EQ(menus.size(), 1u);

  auto const& empty_trash_menu = menus.front();

  unsigned time = g_random_int();
  EXPECT_CALL(*fm_, EmptyTrash(time));
  dbusmenu_menuitem_handle_event(empty_trash_menu, DBUSMENU_MENUITEM_EVENT_ACTIVATED, nullptr, time);
}

TEST_F(TestTrashLauncherIcon, RunningState)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(WindowList({win1, win2})));
  fm_->locations_changed.emit();
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(WindowList()));
  fm_->locations_changed.emit();
  EXPECT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::RUNNING));
}

TEST_F(TestTrashLauncherIcon, ActiveState)
{
  auto win1 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  auto win2 = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(WindowList({win1, win2})));
  fm_->locations_changed.emit();
  ASSERT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));

  win2->LocalFocus();
  EXPECT_TRUE(icon.GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(WindowList()));
  fm_->locations_changed.emit();
  EXPECT_FALSE(icon.GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE));
}

TEST_F(TestTrashLauncherIcon, WindowsCount)
{
  WindowList windows((g_random_int() % 10) + 5);
  for (unsigned i = 0; i < windows.capacity(); ++i)
    windows[i] = std::make_shared<MockApplicationWindow::Nice>(g_random_int());

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(windows));
  fm_->locations_changed.emit();
  EXPECT_EQ(icon.Windows().size(), windows.size());
}

TEST_F(TestTrashLauncherIcon, WindowsPerMonitor)
{
  WindowList windows((g_random_int() % 10) + 5);
  for (unsigned i = 0; i < windows.capacity(); ++i)
  {
    auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
    win->monitor_ = i % 2;
    windows[i] = win;
  }

  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(windows));
  fm_->locations_changed.emit();

  EXPECT_EQ(icon.WindowsVisibleOnMonitor(0), (windows.size() / 2) + (windows.size() % 2));
  EXPECT_EQ(icon.WindowsVisibleOnMonitor(1), windows.size() / 2);
}

TEST_F(TestTrashLauncherIcon, WindowsOnMonitorChanges)
{
  auto win = std::make_shared<MockApplicationWindow::Nice>(g_random_int());
  ON_CALL(*fm_, WindowsForLocation(TRASH_URI)).WillByDefault(Return(WindowList({win})));
  fm_->locations_changed.emit();

  EXPECT_EQ(icon.WindowsVisibleOnMonitor(0), 1u);
  EXPECT_EQ(icon.WindowsVisibleOnMonitor(1), 0u);

  win->SetMonitor(1);
  EXPECT_EQ(icon.WindowsVisibleOnMonitor(0), 0u);
  EXPECT_EQ(icon.WindowsVisibleOnMonitor(1), 1u);
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

TEST_F(TestTrashLauncherIcon, AcceptDropTrashesFiles)
{
  DndData data;
  std::string data_string = "file1\nfile2\ndir3/file3\nfileN";
  data.Fill(data_string.c_str());

  for (auto const& uri : data.Uris())
    EXPECT_CALL(*fm_, TrashFile(uri));

  icon.AcceptDrop(data);
}

MATCHER_P(ApplicationSubjectEquals, other, "") { return *arg == *other; }

TEST_F(TestTrashLauncherIcon, AcceptDropTrashedFilesLogsEvents)
{
  DndData data;
  std::string data_string = "file1\ndir2/file2\nfile3\ndirN/fileN";
  data.Fill(data_string.c_str());
  EXPECT_CALL(*fm_, TrashFile(_)).WillRepeatedly(Return(true));
  std::vector<ApplicationSubjectPtr> subjects;

  for (auto const& uri : data.Uris())
  {
    auto subject = std::make_shared<testmocks::MockApplicationSubject>();
    subject->uri = uri;
    subject->origin = glib::gchar_to_string(g_path_get_dirname(uri.c_str()));
    subject->text = glib::gchar_to_string(g_path_get_basename(uri.c_str()));
    subjects.push_back(subject);
    EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::DELETE, ApplicationSubjectEquals(subject)));
  }

  icon.AcceptDrop(data);

  EXPECT_FALSE(unity_app_->actions_log_.empty());
  for (auto const& subject : subjects)
    ASSERT_TRUE(unity_app_->HasLoggedEvent(ApplicationEventType::DELETE, subject));
}

TEST_F(TestTrashLauncherIcon, AcceptDropFailsDoesNotLogEvents)
{
  DndData data;
  std::string data_string = "file1\ndir2/file2\nfile3\ndirN/fileN";
  data.Fill(data_string.c_str());
  EXPECT_CALL(*fm_, TrashFile(_)).WillRepeatedly(Return(false));
  std::vector<ApplicationSubjectPtr> subjects;

  EXPECT_CALL(*unity_app_, LogEvent(ApplicationEventType::DELETE, _)).Times(0);

  icon.AcceptDrop(data);

  EXPECT_TRUE(unity_app_->actions_log_.empty());
  for (auto const& subject : subjects)
    ASSERT_FALSE(unity_app_->HasLoggedEvent(ApplicationEventType::DELETE, subject));
}

}
