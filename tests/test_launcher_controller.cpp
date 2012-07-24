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
 * Authored by: Marco Trevisan (Treviño) <marco.trevisan@canonical.com>
 *
 */

#include <gmock/gmock.h>
#include "test_utils.h"

#include "PanelStyle.h"
#include "UnitySettings.h"
#include "FavoriteStore.h"
#include "LauncherController.h"

using namespace unity;
using namespace unity::launcher;
using namespace testing;

namespace
{

class MockFavoriteStore : public FavoriteStore
{
public:
  FavoriteList const& GetFavorites()
  {
    return fav_list_;
  };

  void AddFavorite(std::string const& desktop_path, int position) {};
  void RemoveFavorite(std::string const& desktop_path) {};
  void MoveFavorite(std::string const& desktop_path, int position) {};
  void SetFavorites(FavoriteList const& desktop_paths) {};

private:
  FavoriteList fav_list_;
};

class TestLauncherController : public Test
{
public:
  TestLauncherController()
    : lc(nux::GetGraphicsDisplay()->GetX11Display())
  {}

  virtual void SetUp()
  {
    lc.options = std::make_shared<Options>();
    lc.multiple_launchers = true;
  }

  Settings settings;
  panel::Style panel_style;
  MockFavoriteStore favorite_store;
  Controller lc;
};

TEST_F(TestLauncherController, Construction)
{
  EXPECT_NE(lc.options(), nullptr);
  EXPECT_TRUE(lc.multiple_launchers());
}

}
