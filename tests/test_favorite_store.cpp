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
 */

#include <gmock/gmock.h>
#include <config.h>

#include "FavoriteStore.h"

using namespace unity;

namespace
{
class MockFavoriteStore : public FavoriteStore
{
public:
  FavoriteList const& GetFavorites() const  { return fav_list_; }
  void AddFavorite(std::string const& icon_uri, int position) {}
  void RemoveFavorite(std::string const& icon_uri) {}
  void MoveFavorite(std::string const& icon_uri, int position) {}
  bool IsFavorite(std::string const& icon_uri) const { return false; }
  int FavoritePosition(std::string const& icon_uri) const { return -1; }
  void SetFavorites(FavoriteList const& icon_uris) {}

  std::string ParseFavoriteFromUri(std::string const& uri) const
  {
    return FavoriteStore::ParseFavoriteFromUri(uri);
  }

private:
  FavoriteList fav_list_;
};

struct TestFavoriteStore : public testing::Test
{
  MockFavoriteStore favorite_store;
};

TEST_F(TestFavoriteStore, Construction)
{
  FavoriteStore& instance = FavoriteStore::Instance();
  EXPECT_EQ(&instance, &favorite_store);
}

TEST_F(TestFavoriteStore, UriPrefixes)
{
  EXPECT_EQ(FavoriteStore::URI_PREFIX_APP, "application://");
  EXPECT_EQ(FavoriteStore::URI_PREFIX_FILE, "file://");
  EXPECT_EQ(FavoriteStore::URI_PREFIX_DEVICE, "device://");
  EXPECT_EQ(FavoriteStore::URI_PREFIX_UNITY, "unity://");
}

TEST_F(TestFavoriteStore, IsValidFavoriteUri)
{
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri(""));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("invalid-favorite"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("/path/to/desktop_file"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("desktop_file"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("file:///path/to/desktop_file"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("application://desktop_file"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("application://"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("device://"));
  EXPECT_FALSE(FavoriteStore::IsValidFavoriteUri("unity://"));

  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("device://uuid"));
  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("file:///path/to/desktop_file.desktop"));
  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("application://desktop_file.desktop"));
  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("device://a"));
  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("unity://b"));
  EXPECT_TRUE(FavoriteStore::IsValidFavoriteUri("application://c.desktop"));
}

TEST_F(TestFavoriteStore, ParseFavoriteFromUri)
{
  const std::string VALID_DESKTOP_PATH = BUILDDIR"/tests/data/applications/ubuntu-software-center.desktop";
  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("file.desktop"), "application://file.desktop");
  EXPECT_EQ(favorite_store.ParseFavoriteFromUri(VALID_DESKTOP_PATH), "application://"+VALID_DESKTOP_PATH);

  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("application://file.desktop"), "application://file.desktop");
  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("application://"+VALID_DESKTOP_PATH), "application://"+VALID_DESKTOP_PATH);
  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("file://"+VALID_DESKTOP_PATH), "file://"+VALID_DESKTOP_PATH);

  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("unity://uri"), "unity://uri");
  EXPECT_EQ(favorite_store.ParseFavoriteFromUri("device://uuid"), "device://uuid");

  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("file").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("/path/to/file").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("/path/to/file.desktop").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("application:///path/to/file.desktop").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("file:///path/to/file.desktop").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("file://file.desktop").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("unity://").empty());
  EXPECT_TRUE(favorite_store.ParseFavoriteFromUri("device://").empty());
}

}
