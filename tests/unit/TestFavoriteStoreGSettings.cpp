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
 *
 */


#include "config.h"

#include <algorithm>

#include <gio/gio.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "FavoriteStore.h"
#include "FavoriteStoreGSettings.h"

#define CUSTOM_DESKTOP BUILDDIR"/tests/data/update-manager.desktop"

#define BASE_STORE_FILE BUILDDIR"/settings/test-favorite-store-gsettings.store"
#define BASE_STORE_CONTENTS "[desktop/unity/launcher]\n" \
                            "favorites=['evolution.desktop', 'firefox.desktop', '%s']"

namespace
{

const char* base_store_favs[] = { "evolution.desktop",
                                  "firefox.desktop",
                                  CUSTOM_DESKTOP,
                                  NULL
                                };
const int n_base_store_favs = G_N_ELEMENTS(base_store_favs) - 1; /* NULL */

void TestAllocation();
void TestGetFavorites();

void TestAddFavorite();
void TestAddFavoritePosition();
void TestAddFavoriteLast();
void TestAddFavoriteOutOfRange();

void TestRemoveFavorite();
void TestRemoveFavoriteBad();

void TestMoveFavorite();
void TestMoveFavoriteBad();
}

using namespace unity;

void TestFavoriteStoreGSettingsCreateSuite()
{
#define TEST_DOMAIN "/Unit/FavoriteStoreGSettings"
  g_test_add_func(TEST_DOMAIN"/Allocation", TestAllocation);
  g_test_add_func(TEST_DOMAIN"/GetFavorites", TestGetFavorites);
  g_test_add_func(TEST_DOMAIN"/AddFavorite", TestAddFavorite);
  g_test_add_func(TEST_DOMAIN"/AddFavoritePosition", TestAddFavoritePosition);
  g_test_add_func(TEST_DOMAIN"/AddFavoriteLast", TestAddFavoriteLast);
  g_test_add_func(TEST_DOMAIN"/AddFavoriteOutOfRange", TestAddFavoriteOutOfRange);
  g_test_add_func(TEST_DOMAIN"/RemoveFavorite", TestRemoveFavorite);
  g_test_add_func(TEST_DOMAIN"/RemoveFavoriteBad", TestRemoveFavoriteBad);
  g_test_add_func(TEST_DOMAIN"/MoveFavorite", TestMoveFavorite);
  g_test_add_func(TEST_DOMAIN"/MoveFavoriteBad", TestMoveFavoriteBad);
#undef TEST_DOMAIN
}

namespace
{

GSettingsBackend* CreateDefaultKeyFileBackend()
{
  GSettingsBackend* b;
  GError* error = NULL;
  gchar*  contents = NULL;

  contents = g_strdup_printf(BASE_STORE_CONTENTS, CUSTOM_DESKTOP);

  g_file_set_contents(BASE_STORE_FILE,
                      contents,
                      -1,
                      &error);
  g_assert(error == NULL);

  b = g_keyfile_settings_backend_new(BASE_STORE_FILE, "/", "root");

  g_free(contents);
  return b;
}

bool ends_with(std::string const& value, std::string const& suffix)
{
  std::string::size_type pos = value.rfind(suffix);
  if (pos == std::string::npos)
    return false;
  else
    return (pos + suffix.size()) == value.size();
}

std::string const& at(FavoriteList const& favs, int index)
{
  FavoriteList::const_iterator iter = favs.begin();
  std::advance(iter, index);
  return *iter;
}

void TestAllocation()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  g_assert(G_IS_SETTINGS_BACKEND(backend.RawPtr()));
}

void TestGetFavorites()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());

  FavoriteList const& favs = settings.GetFavorites();
  g_assert_cmpint(favs.size(), == , 3);
  g_assert(ends_with(at(favs, 0), base_store_favs[0]));
  g_assert(ends_with(at(favs, 1), base_store_favs[1]));
  g_assert(at(favs, 2) == base_store_favs[2]);
}

void TestAddFavorite()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());
  std::string other_desktop("/usr/share/applications/nautilus.desktop");

  settings.AddFavorite(other_desktop, 0);
  FavoriteList const& favs = settings.GetFavorites();
  g_assert(other_desktop == at(favs, 0));
  g_assert_cmpint(favs.size(), == , n_base_store_favs + 1);
}

void TestAddFavoritePosition()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());
  std::string other_desktop("/usr/share/applications/nautilus.desktop");

  settings.AddFavorite(other_desktop, 2);
  FavoriteList const& favs = settings.GetFavorites();
  g_assert(other_desktop == at(favs, 2));
  g_assert_cmpint(favs.size(), == , n_base_store_favs + 1);
}

void TestAddFavoriteLast()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());
  std::string other_desktop("/usr/share/applications/nautilus.desktop");

  settings.AddFavorite(other_desktop, -1);
  FavoriteList const& favs = settings.GetFavorites();
  g_assert(other_desktop == favs.back());
  g_assert_cmpint(favs.size(), == , n_base_store_favs + 1);
}

void TestAddFavoriteOutOfRange()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());
  std::string other_desktop("/usr/share/applications/nautilus.desktop");

  FavoriteList const& favs = settings.GetFavorites();
  settings.AddFavorite(other_desktop, n_base_store_favs + 1);
  // size didn't change
  g_assert_cmpint(favs.size(), == , n_base_store_favs);
  // didn't get inserted
  FavoriteList::const_iterator iter = std::find(favs.begin(), favs.end(), other_desktop);
  g_assert(iter == favs.end());
}

void TestRemoveFavorite()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());

  FavoriteList const& favs = settings.GetFavorites();
  settings.RemoveFavorite(at(favs, 0));

  g_assert_cmpint(favs.size(), == , n_base_store_favs - 1);
  g_assert(ends_with(at(favs, 0), base_store_favs[1]));

  settings.RemoveFavorite(at(favs, 1));
  g_assert_cmpint(favs.size(), == , n_base_store_favs - 2);
  g_assert(ends_with(at(favs, 0), base_store_favs[1]));
}

void TestRemoveFavoriteBad()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());

  FavoriteList const& favs = settings.GetFavorites();
  settings.RemoveFavorite("");
  g_assert_cmpint(favs.size(), == , n_base_store_favs);

  settings.RemoveFavorite("foo.desktop");
  g_assert_cmpint(favs.size(), == , n_base_store_favs);

  settings.RemoveFavorite("/this/desktop/doesnt/exist/hopefully.desktop");
  g_assert_cmpint(favs.size(), == , n_base_store_favs);
}

void TestMoveFavorite()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());

  FavoriteList const& favs = settings.GetFavorites();

  settings.MoveFavorite(base_store_favs[2], 0);

  g_assert_cmpint(favs.size(), == , n_base_store_favs);
  g_assert(at(favs, 0) == base_store_favs[2]);
  g_assert(ends_with(at(favs, 1), base_store_favs[0]));
  g_assert(ends_with(at(favs, 2), base_store_favs[1]));
}

void TestMoveFavoriteBad()
{
  glib::Object<GSettingsBackend> backend(CreateDefaultKeyFileBackend());
  internal::FavoriteStoreGSettings settings(backend.Release());

  FavoriteList const& favs = settings.GetFavorites();

  settings.MoveFavorite("", 0);
  settings.MoveFavorite(at(favs, 0), 100);

  g_assert_cmpint(favs.size(), == , 3);
  g_assert(ends_with(at(favs, 0), base_store_favs[0]));
  g_assert(ends_with(at(favs, 1), base_store_favs[1]));
  g_assert(at(favs, 2) == base_store_favs[2]);
}

} // anon namespace
