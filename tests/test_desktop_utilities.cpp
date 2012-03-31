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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *              Tim Penhey <tim.penhey@canonical.com>
 */

#include <UnityCore/DesktopUtilities.h>

#include <glib.h>
#include <gmock/gmock.h>

using namespace unity;
using testing::Eq;

namespace {
  
TEST(TestDesktopUtilitiesDesktopID, TestEmptyValues)
{
  std::vector<std::string> empty_list;

  EXPECT_THAT(DesktopUtilities::GetDesktopID(empty_list, "/some/path/to.desktop"),
              Eq("/some/path/to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(empty_list, "/some/path/to.desktop"),
              Eq("/some/path/to.desktop"));

  std::vector<std::string> empty_path_list;
  empty_path_list.push_back("");

  EXPECT_THAT(DesktopUtilities::GetDesktopID(empty_path_list, "/some/path/to.desktop"),
              Eq("/some/path/to.desktop"));
}

TEST(TestDesktopUtilitiesDesktopID, TestPathNeedsApplications)
{
  std::vector<std::string> dirs;

  dirs.push_back("/this/path");
  dirs.push_back("/that/path/");

  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/path/to.desktop"),
              Eq("/this/path/to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/that/path/to.desktop"),
              Eq("/that/path/to.desktop"));
}

TEST(TestDesktopUtilitiesDesktopID, TestStripsPath)
{
  std::vector<std::string> dirs;

  dirs.push_back("/this/path");
  dirs.push_back("/that/path/");

  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/path/applications/to.desktop"),
              Eq("to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/that/path/applications/to.desktop"),
              Eq("to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/some/path/applications/to.desktop"),
              Eq("/some/path/applications/to.desktop"));
}

TEST(TestDesktopUtilitiesDesktopID, TestSubdirectory)
{
  std::vector<std::string> dirs;

  dirs.push_back("/this/path");
  dirs.push_back("/that/path/");

  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/path/applications/subdir/to.desktop"),
              Eq("subdir-to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/that/path/applications/subdir/to.desktop"),
              Eq("subdir-to.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/path/applications/subdir1/subdir2/to.desktop"),
              Eq("subdir1-subdir2-to.desktop"));
}

TEST(TestDesktopUtilitiesDataDirectories, TestGetUserDataDirectory)
{
  const gchar* old_home = g_getenv("XDG_DATA_HOME");
  g_setenv("XDG_DATA_HOME", "UnityUserConfig", TRUE);

  std::string const& user_data_dir = DesktopUtilities::GetUserDataDirectory();

  g_setenv("XDG_DATA_HOME", old_home, TRUE);

  EXPECT_THAT(user_data_dir, Eq("UnityUserConfig"));
}

TEST(TestDesktopUtilitiesDataDirectories, TestGetSystemDataDirectory)
{
  const gchar* old_dirs = g_getenv("XDG_DATA_DIRS");
  g_setenv("XDG_DATA_DIRS", "dir1:dir2::dir3:dir4", TRUE);

  std::vector<std::string> const& system_dirs = DesktopUtilities::GetSystemDataDirectories();

  g_setenv("XDG_DATA_DIRS", old_dirs, TRUE);

  ASSERT_THAT(system_dirs.size(), Eq(4));
  EXPECT_THAT(system_dirs[0], Eq("dir1"));
  EXPECT_THAT(system_dirs[1], Eq("dir2"));
  EXPECT_THAT(system_dirs[2], Eq("dir3"));
  EXPECT_THAT(system_dirs[3], Eq("dir4"));
}

TEST(TestDesktopUtilitiesDataDirectories, TestGetDataDirectory)
{
  const gchar* old_dirs = g_getenv("XDG_DATA_DIRS");
  g_setenv("XDG_DATA_DIRS", "dir1:dir2::dir3:dir4", TRUE);
  const gchar* old_home = g_getenv("XDG_DATA_HOME");
  g_setenv("XDG_DATA_HOME", "UnityUserConfig", TRUE);

  std::vector<std::string> const& data_dirs = DesktopUtilities::GetDataDirectories();

  g_setenv("XDG_DATA_DIRS", old_dirs, TRUE);
  g_setenv("XDG_DATA_HOME", old_home, TRUE);

  ASSERT_THAT(data_dirs.size(), Eq(5));
  EXPECT_THAT(data_dirs[0], Eq("dir1"));
  EXPECT_THAT(data_dirs[1], Eq("dir2"));
  EXPECT_THAT(data_dirs[2], Eq("dir3"));
  EXPECT_THAT(data_dirs[3], Eq("dir4"));
  EXPECT_THAT(data_dirs[4], Eq("UnityUserConfig"));
}

} // anonymous namespace
