/*
 * Copyright 2010 Canonical Ltd.
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Andrea Azzarone <azzaronea@gmail.com>
 *
 */

#include <config.h>
#include <gmock/gmock.h>
#include <glib.h>

#include "test_utils.h"
#include "FavoriteStore.h"
#include "FavoriteStoreGSettings.h"

#include <UnityCore/GLibWrapper.h>

using namespace unity;
using testing::Eq;

namespace {

// Constant
const gchar* SETTINGS_NAME = "com.canonical.Unity.Launcher";
const gchar* SETTINGS_KEY = "favorites";

const char* base_store_favs[] = { BUILDDIR"/tests/data/applications/ubuntuone-installer.desktop",
                                  "file://" BUILDDIR "/tests/data/applications/ubuntu-software-center.desktop",
                                  "application://" BUILDDIR "/tests/data/applications/update-manager.desktop",
                                  "unity://test-icon",
                                  "device://uuid",
                                  NULL
                                };
const int n_base_store_favs = G_N_ELEMENTS(base_store_favs) - 1; /* NULL */

const std::string other_desktop = "application://" BUILDDIR "/tests/data/applications/bzr-handle-patch.desktop";

// Utilities
std::string const& at(FavoriteList const& favs, int index)
{
  FavoriteList::const_iterator iter = favs.begin();
  std::advance(iter, index);
  return *iter;
}

bool ends_with(std::string const& value, std::string const& suffix)
{
  std::string::size_type pos = value.rfind(suffix);
  if (pos == std::string::npos)
    return false;
  else
    return (pos + suffix.size()) == value.size();
}

// A new one of these is created for each test
class TestFavoriteStoreGSettings : public testing::Test
{
public:
  std::unique_ptr<internal::FavoriteStoreGSettings> favorite_store;
  glib::Object<GSettings> gsettings_client;

  virtual void SetUp()
  {
    // set the data directory so gsettings can find the schema
    Utils::init_gsettings_test_environment();

    favorite_store.reset(new internal::FavoriteStoreGSettings());

    // Setting the test values
    gsettings_client = g_settings_new(SETTINGS_NAME);
    g_settings_set_strv(gsettings_client, SETTINGS_KEY, base_store_favs);
  }

  virtual void TearDown()
  {
    Utils::reset_gsettings_test_environment();
  }
};

TEST_F(TestFavoriteStoreGSettings, TestAllocation)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  EXPECT_EQ(&settings, favorite_store.get());
}

TEST_F(TestFavoriteStoreGSettings, TestGetFavorites)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  FavoriteList const& favs = settings.GetFavorites();

  ASSERT_EQ(favs.size(), n_base_store_favs);
  EXPECT_TRUE(ends_with(at(favs, 0), FavoriteStore::URI_PREFIX_APP+base_store_favs[0]));
  EXPECT_TRUE(ends_with(at(favs, 1), base_store_favs[1]));
  EXPECT_EQ(at(favs, 2), base_store_favs[2]);
}

TEST_F(TestFavoriteStoreGSettings, TestAddFavorite)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  settings.AddFavorite(other_desktop, 0);
  FavoriteList const& favs = settings.GetFavorites();
  ASSERT_EQ(favs.size(), n_base_store_favs + 1);
  EXPECT_EQ(other_desktop, at(favs, 0));
}

TEST_F(TestFavoriteStoreGSettings, TestAddFavoritePosition)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  settings.AddFavorite(other_desktop, 2);
  FavoriteList const& favs = settings.GetFavorites();
  ASSERT_EQ(favs.size(), n_base_store_favs + 1);
  EXPECT_EQ(other_desktop, at(favs, 2));
}

TEST_F(TestFavoriteStoreGSettings,TestAddFavoriteLast)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  settings.AddFavorite(other_desktop, -1);
  FavoriteList const& favs = settings.GetFavorites();
  ASSERT_EQ(favs.size(), n_base_store_favs + 1);
  EXPECT_EQ(other_desktop, favs.back());
}

TEST_F(TestFavoriteStoreGSettings,TestAddFavoriteOutOfRange)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  FavoriteList const& favs = settings.GetFavorites();
  settings.AddFavorite(other_desktop, n_base_store_favs + 1);
  // size didn't change
  ASSERT_EQ(favs.size(), n_base_store_favs);
  // didn't get inserted
  FavoriteList::const_iterator iter = std::find(favs.begin(), favs.end(), other_desktop);
  EXPECT_EQ(iter, favs.end());
}

TEST_F(TestFavoriteStoreGSettings, TestRemoveFavorite)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  FavoriteList const& favs = settings.GetFavorites();
  settings.RemoveFavorite(at(favs, 0));

  ASSERT_EQ(favs.size(), n_base_store_favs - 1);
  EXPECT_TRUE(ends_with(at(favs, 0), base_store_favs[1]));

  settings.RemoveFavorite(at(favs, 1));
  ASSERT_EQ(favs.size(), n_base_store_favs - 2);
  EXPECT_TRUE(ends_with(at(favs, 0), base_store_favs[1]));
}

TEST_F(TestFavoriteStoreGSettings, TestRemoveFavoriteBad)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  FavoriteList const& favs = settings.GetFavorites();
  settings.RemoveFavorite(" ");
  EXPECT_EQ(favs.size(), n_base_store_favs);

  settings.RemoveFavorite("application://foo.desktop");
  EXPECT_EQ(favs.size(), n_base_store_favs);

  settings.RemoveFavorite("application:///this/desktop/doesnt/exist/hopefully.desktop");
  EXPECT_EQ(favs.size(), n_base_store_favs);
}

TEST_F(TestFavoriteStoreGSettings, TestMoveFavorite)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  FavoriteList const& favs = settings.GetFavorites();

  settings.MoveFavorite(base_store_favs[2], 0);

  ASSERT_EQ(favs.size(), n_base_store_favs);
  EXPECT_EQ(at(favs, 0), base_store_favs[2]);
  EXPECT_TRUE(ends_with(at(favs, 1), FavoriteStore::URI_PREFIX_APP+base_store_favs[0]));
  EXPECT_TRUE(ends_with(at(favs, 2), base_store_favs[1]));
}

TEST_F(TestFavoriteStoreGSettings, TestMoveFavoriteBad)
{
  FavoriteStore &settings = FavoriteStore::Instance();

  FavoriteList const& favs = settings.GetFavorites();

  settings.MoveFavorite("", 0);
  settings.MoveFavorite(at(favs, 0), 100);

  ASSERT_EQ(favs.size(), n_base_store_favs);
  EXPECT_TRUE(ends_with(at(favs, 0), FavoriteStore::URI_PREFIX_APP+base_store_favs[0]));
  EXPECT_TRUE(ends_with(at(favs, 1), base_store_favs[1]));
  EXPECT_EQ(at(favs, 2), base_store_favs[2]);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteAddedSignalFirst)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;
  std::string position;
  bool before = false;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    signal_received = true;
    position = pos;
    before = bef;
  });

  FavoriteList favs;
  favs.push_back(other_desktop);
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[1]);
  favs.push_back(base_store_favs[2]);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);
  EXPECT_EQ(position, FavoriteStore::URI_PREFIX_APP+base_store_favs[0]);
  EXPECT_TRUE(before);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteAddedSignalMiddle)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;
  std::string position;
  bool before = true;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    signal_received = true;
    position = pos;
    before = bef;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[1]);
  favs.push_back(other_desktop);
  favs.push_back(base_store_favs[2]);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);
  EXPECT_EQ(position, base_store_favs[1]);
  EXPECT_FALSE(before);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteAddedSignalEnd)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;
  std::string position;
  bool before = true;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    signal_received = true;
    position = pos;
    before = bef;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[1]);
  favs.push_back(base_store_favs[2]);
  favs.push_back(other_desktop);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);
  EXPECT_EQ(position, base_store_favs[2]);
  EXPECT_FALSE(before);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteAddedSignalEmpty)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;
  std::string position;
  bool before = false;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    signal_received = true;
    position = pos;
    before = bef;
  });

  FavoriteList favs;
  favs.push_back(other_desktop);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);
  EXPECT_EQ(position, "");
  EXPECT_TRUE(before);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteRemoved)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;
  std::vector<std::string> paths_removed;

  settings.favorite_removed.connect([&](std::string const& path)
  {
    signal_received = true;
    paths_removed.push_back(path);
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[2]);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);
  ASSERT_EQ(paths_removed.size(), 3);
  EXPECT_EQ(paths_removed[0], base_store_favs[4]);
  EXPECT_EQ(paths_removed[1], base_store_favs[1]);
  EXPECT_EQ(paths_removed[2], base_store_favs[3]);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteReordered)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool signal_received = false;

  settings.reordered.connect([&]()
  {
    signal_received = true;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[2]);
  favs.push_back(base_store_favs[1]);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_TRUE(signal_received);

  signal_received = false;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[2]);
  favs.push_back(base_store_favs[1]);
  favorite_store->SaveFavorites(favs, false);

  ASSERT_FALSE(signal_received);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteSignalsMixed1)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool added_received = false;
  bool removed_received = false;
  bool reordered_received = false;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    added_received = true;
  });

  settings.favorite_removed.connect([&](std::string const& path)
  {
    removed_received = true;
  });

  settings.reordered.connect([&]()
  {
    reordered_received = true;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[0]);
  favs.push_back(base_store_favs[1]);
  favs.push_back(other_desktop);
  favorite_store->SaveFavorites(favs, false);

  EXPECT_TRUE(added_received);
  EXPECT_TRUE(removed_received);
  EXPECT_FALSE(reordered_received);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteSignalsMixed2)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool added_received = false;
  bool removed_received = false;
  bool reordered_received = false;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    added_received = true;
  });

  settings.favorite_removed.connect([&](std::string const& path)
  {
    removed_received = true;
  });

  settings.reordered.connect([&]()
  {
    reordered_received = true;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[1]);
  favs.push_back(other_desktop);
  favs.push_back(base_store_favs[0]);
  favorite_store->SaveFavorites(favs, false);

  EXPECT_TRUE(added_received);
  EXPECT_TRUE(removed_received);
  EXPECT_TRUE(reordered_received);
}

TEST_F(TestFavoriteStoreGSettings, TestFavoriteSignalsMixed3)
{
  FavoriteStore &settings = FavoriteStore::Instance();
  bool added_received = false;
  bool removed_received = false;
  bool reordered_received = false;

  settings.favorite_added.connect([&](std::string const& path, std::string const& pos, bool bef)
  {
    added_received = true;
  });

  settings.favorite_removed.connect([&](std::string const& path)
  {
    removed_received = true;
  });

  settings.reordered.connect([&]()
  {
    reordered_received = true;
  });

  FavoriteList favs;
  favs.push_back(base_store_favs[1]);
  favs.push_back(base_store_favs[0]);
  favorite_store->SaveFavorites(favs, false);

  EXPECT_FALSE(added_received);
  EXPECT_TRUE(removed_received);
  EXPECT_TRUE(reordered_received);
}

TEST_F(TestFavoriteStoreGSettings, TestIsFavorite)
{
  EXPECT_TRUE(favorite_store->IsFavorite(FavoriteStore::URI_PREFIX_APP+base_store_favs[0]));

  for (int i = 1; i < n_base_store_favs; i++)
  {
    ASSERT_TRUE(favorite_store->IsFavorite(base_store_favs[i]));
  }

  EXPECT_FALSE(favorite_store->IsFavorite("unity://invalid-favorite"));
}

TEST_F(TestFavoriteStoreGSettings, TestFavoritePosition)
{
  EXPECT_EQ(favorite_store->FavoritePosition(FavoriteStore::URI_PREFIX_APP+base_store_favs[0]), 0);

  for (int i = 1; i < n_base_store_favs; i++)
  {
    ASSERT_EQ(favorite_store->FavoritePosition(base_store_favs[i]), i);
  }

  EXPECT_EQ(favorite_store->FavoritePosition("unity://invalid-favorite"), -1);
}

} // anonymous namespace
