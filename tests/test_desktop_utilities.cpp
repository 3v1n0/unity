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
 *              Andrea Azzarone <azzarone@gmail.com>
 */

#include <config.h>

#include <UnityCore/DesktopUtilities.h>

#include <glib.h>
#include <gmock/gmock.h>

using namespace unity;
using testing::Eq;

namespace {

const std::string LOCAL_DATA_DIR = BUILDDIR"/tests/data";

TEST(TestDesktopUtilitiesDesktopID, TestEmptyValues)
{
  std::vector<std::string> empty_list;
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

TEST(TestDesktopUtilitiesDesktopID, TestUnescapePath)
{
  std::vector<std::string> dirs;

  dirs.push_back("/this/ path");
  dirs.push_back("/that/path /");

  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/%20path/applications/to%20file.desktop"),
              Eq("to file.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/that/path%20/applications/to%20file.desktop"),
              Eq("to file.desktop"));
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/some/path/applications/to%20file.desktop"),
              Eq("/some/path/applications/to file.desktop"));
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
  EXPECT_THAT(DesktopUtilities::GetDesktopID(dirs, "/this/path/applications/subdir1/subdir2-to.desktop"),
              Eq("subdir1-subdir2-to.desktop"));
}

// We can't test this. GetUserDataDirectory uses g_get_user_data_dir which caches data dir.
// If we perform a DesktopUtilities::GetUserDataDirectory(); before running this test, the test will fail, because setting the XDG_DATA_HOME
// will not change the cached value.

// TEST(TestDesktopUtilitiesDataDirectories, TestGetUserDataDirectory)
// {
//   const gchar* env = g_getenv("XDG_DATA_HOME");
//   std::string old_home = env ? env : "";

//   g_setenv("XDG_DATA_HOME", "UnityUserConfig", TRUE);

//   std::string const& user_data_dir = DesktopUtilities::GetUserDataDirectory();

//   g_setenv("XDG_DATA_HOME", old_home.c_str(), TRUE);

//   EXPECT_THAT(user_data_dir, Eq("UnityUserConfig"));
// }

// We can't test this. GetSystemDataDirectories uses g_get_system_data_dirs which caches the values.
// If we perform a DesktopUtilities::GetSystemDataDirectories(); before running this test, the test will fail, because setting the XDG_DATA_DIRS
// will not change the cached value.

// TEST(TestDesktopUtilitiesDataDirectories, TestGetSystemDataDirectory)
// {
//   const gchar* env = g_getenv("XDG_DATA_DIRS");
//   std::string old_dirs = env ? env : "";
//   g_setenv("XDG_DATA_DIRS", ("dir1:dir2::dir3:dir4:"+LOCAL_DATA_DIR).c_str(), TRUE);

//   std::vector<std::string> const& system_dirs = DesktopUtilities::GetSystemDataDirectories();

//   g_setenv("XDG_DATA_DIRS", old_dirs.c_str(), TRUE);

//   ASSERT_THAT(system_dirs.size(), Eq(5));
//   EXPECT_THAT(system_dirs[0], Eq("dir1"));
//   EXPECT_THAT(system_dirs[1], Eq("dir2"));
//   EXPECT_THAT(system_dirs[2], Eq("dir3"));
//   EXPECT_THAT(system_dirs[3], Eq("dir4"));
//   EXPECT_THAT(system_dirs[4], Eq(LOCAL_DATA_DIR));
// }

// We can't test this. TestGetDataDirectory uses g_get_system_data_dirs which caches the values.
// If we perform a DesktopUtilities::TestGetDataDirectory(); before running this test, the test will fail, because setting the XDG_DATA_DIRS or XDG_DATA_HOME
// will not change the cached value.

// TEST(TestDesktopUtilitiesDataDirectories, TestGetDataDirectory)
// {
//   const gchar* env = g_getenv("XDG_DATA_DIRS");
//   std::string old_dirs = env ? env : "";
//   env = g_getenv("XDG_DATA_HOME");
//   std::string old_home = env ? env : "";

//   g_setenv("XDG_DATA_DIRS", ("dir1:dir2::dir3:dir4:"+LOCAL_DATA_DIR).c_str(), TRUE);
//   g_setenv("XDG_DATA_HOME", "UnityUserConfig", TRUE);

//   std::vector<std::string> const& data_dirs = DesktopUtilities::GetDataDirectories();

//   g_setenv("XDG_DATA_DIRS", old_dirs.c_str(), TRUE);
//   g_setenv("XDG_DATA_HOME", old_home.c_str(), TRUE);

//   ASSERT_THAT(data_dirs.size(), Eq(6));
//   EXPECT_THAT(data_dirs[0], Eq("dir1"));
//   EXPECT_THAT(data_dirs[1], Eq("dir2"));
//   EXPECT_THAT(data_dirs[2], Eq("dir3"));
//   EXPECT_THAT(data_dirs[3], Eq("dir4"));
//   EXPECT_THAT(data_dirs[4], Eq(LOCAL_DATA_DIR));
//   EXPECT_THAT(data_dirs[5], Eq("UnityUserConfig"));
// }

TEST(TestDesktopUtilities, TestGetDesktopPathById)
{
  const gchar* env = g_getenv("XDG_DATA_DIRS");
  std::string old_dirs = env ? env : "";
  env = g_getenv("XDG_DATA_HOME");
  std::string old_home = env ? env : "";

  g_setenv("XDG_DATA_DIRS", LOCAL_DATA_DIR.c_str(), TRUE);
  g_setenv("XDG_DATA_HOME", "UnityUserConfig", TRUE);

  std::string const& file = DesktopUtilities::GetDesktopPathById("ubuntu-software-center.desktop");

  g_setenv("XDG_DATA_DIRS", old_dirs.c_str(), TRUE);
  g_setenv("XDG_DATA_HOME", old_dirs.c_str(), TRUE);

  EXPECT_EQ(file, LOCAL_DATA_DIR + "/applications/ubuntu-software-center.desktop");
}

TEST(TestDesktopUtilities, TestGetBackgroundColor)
{
  std::string const& color = DesktopUtilities::GetBackgroundColor(LOCAL_DATA_DIR+"/applications/ubuntu-software-center.desktop");

  EXPECT_EQ(color, "#aabbcc");
}

TEST(TestDesktopUtilities, TestGetBackgroundColorNoKey)
{
  std::string const& color = DesktopUtilities::GetBackgroundColor(LOCAL_DATA_DIR+"/applications/update-manager.desktop");

  EXPECT_TRUE(color.empty());
}

TEST(TestDesktopUtilities, TestGetBackgroundColorNoFile)
{
  std::string const& color = DesktopUtilities::GetBackgroundColor("hello-world.desktop");

  EXPECT_TRUE(color.empty());
}

} // anonymous namespace
