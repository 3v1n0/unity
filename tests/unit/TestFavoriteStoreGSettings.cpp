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

#include <gio/gio.h>
#define G_SETTINGS_ENABLE_BACKEND
#include <gio/gsettingsbackend.h>

#include "FavoriteStore.h"
#include "FavoriteStoreGSettings.h"

#define CUSTOM_DESKTOP BUILDDIR"/tests/data/update-manager.desktop"

#define BASE_STORE_FILE BUILDDIR"/settings/test-favorite-store-gsettings.store"
#define BASE_STORE_CONTENTS "[desktop/unity/launcher]\n" \
                            "favorites=['evolution.desktop', 'firefox.desktop', '%s']"

static const char *base_store_favs[] = { "evolution.desktop",
                                          "firefox.desktop",
                                          CUSTOM_DESKTOP,
                                          NULL };
static int         n_base_store_favs = G_N_ELEMENTS (base_store_favs) - 1; /* NULL */

static void TestAllocation   (void);
static void TestGetFavorites (void);
static void TestAddFavorite  (void);
static void TestAddFavoritePosition (void);
static void TestAddFavoriteLast (void);
static void TestRemoveFavorite (void);
static void TestRemoveFavoriteBad (void);

void
TestFavoriteStoreGSettingsCreateSuite ()
{
#define _DOMAIN "/Unit/FavoriteStoreGSettings"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
  
  g_test_add_func (_DOMAIN"/GetFavorites", TestGetFavorites);
  
  g_test_add_func (_DOMAIN"/AddFavorite", TestAddFavorite);
  g_test_add_func (_DOMAIN"/AddFavoritePosition", TestAddFavoritePosition);
  g_test_add_func (_DOMAIN"/AddFavoriteLast", TestAddFavoriteLast);

  g_test_add_func (_DOMAIN"/RemoveFavorite", TestRemoveFavorite);
  g_test_add_func (_DOMAIN"/RemoveFavoriteBad", TestRemoveFavoriteBad);
}

static GSettingsBackend *
CreateDefaultKeyFileBackend ()
{
  GSettingsBackend *b;
  GError *error = NULL;
  gchar  *contents = NULL;

  contents = g_strdup_printf (BASE_STORE_CONTENTS, CUSTOM_DESKTOP);

  g_file_set_contents (BASE_STORE_FILE,
                       contents,
                       -1,
                       &error);
  g_assert (error == NULL);

  b = g_keyfile_settings_backend_new (BASE_STORE_FILE, "/", "root");

  g_free (contents);
  return b;
}

static void
TestAllocation ()
{
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;

  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  settings->UnReference ();
}

static void
TestGetFavorites ()
{
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
  GSList *favs, *f;
  int     i = 0;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  favs = settings->GetFavorites ();

  g_assert_cmpint (g_slist_length (favs), ==, n_base_store_favs);

  for (f = favs; f; f = f->next, i++)
    {
      gchar *basename;
      
      basename = g_path_get_basename ((char*)f->data);
      if (g_strcmp0 (basename, "update-manager.desktop") == 0)
        g_assert_cmpstr (CUSTOM_DESKTOP, ==, base_store_favs[i]);
      else
        g_assert_cmpstr (basename, ==, base_store_favs[i]);

      g_free (basename);
    }

  settings->UnReference ();
}

static void
TestAddFavorite ()
{
#define OTHER_DESKTOP "/usr/share/applications/nautilus.desktop"
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
  GSList                 *favs;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  settings->AddFavorite (OTHER_DESKTOP, 0);

  favs = settings->GetFavorites ();
  g_assert_cmpstr ((const gchar *)g_slist_nth_data (favs, 0), ==, OTHER_DESKTOP);

  settings->UnReference ();
}

static void
TestAddFavoritePosition ()
{
#define OTHER_DESKTOP "/usr/share/applications/nautilus.desktop"
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
  GSList                 *favs;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  settings->AddFavorite (OTHER_DESKTOP, 2);

  favs = settings->GetFavorites ();
  g_assert_cmpstr ((const gchar *)g_slist_nth_data (favs, 2), ==, OTHER_DESKTOP);

  settings->UnReference ();
}

static void
TestAddFavoriteLast ()
{
#define OTHER_DESKTOP "/usr/share/applications/nautilus.desktop"
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
  GSList                 *favs;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  settings->AddFavorite (OTHER_DESKTOP, -1);

  favs = settings->GetFavorites ();
  g_assert_cmpstr ((const gchar *)g_slist_nth_data (favs, n_base_store_favs), ==, OTHER_DESKTOP);

  settings->UnReference ();
}

static void
TestRemoveFavorite ()
{
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
  GSList                 *favs;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  favs = settings->GetFavorites ();

  settings->RemoveFavorite ((const gchar *)g_slist_nth_data (favs, 0));
  favs = settings->GetFavorites ();
  g_assert_cmpint (g_slist_length (favs), ==, n_base_store_favs - 1);
  g_assert (g_str_has_suffix ((const gchar *)g_slist_nth_data (favs, 0), base_store_favs[1]));

  settings->RemoveFavorite ((const char *)g_slist_nth_data (favs, 1));
  favs = settings->GetFavorites ();
  g_assert_cmpint (g_slist_length (favs), ==, n_base_store_favs - 2);
  g_assert (g_str_has_suffix ((const gchar *)g_slist_nth_data (favs, 0), base_store_favs[1]));

  settings->UnReference ();
}

static void
TestRemoveFavoriteBad ()
{
  GSettingsBackend       *backend;
  FavoriteStoreGSettings *settings;
 
  backend = CreateDefaultKeyFileBackend ();
  g_assert (G_IS_SETTINGS_BACKEND (backend));

  settings = new FavoriteStoreGSettings (backend);
  g_assert (settings != NULL);
  g_object_unref (backend);

  if (g_test_trap_fork (0, (GTestTrapFlags)(G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)))
    {
      settings->RemoveFavorite (NULL);
    }
  g_test_trap_assert_stderr ("*");

  if (g_test_trap_fork (0, (GTestTrapFlags)(G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)))
    {
      settings->RemoveFavorite ("foo.desktop");
    }
  g_test_trap_assert_stderr ("*[0]*");

  if (g_test_trap_fork (0, (GTestTrapFlags)(G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR)))
    {
      settings->RemoveFavorite ("/this/desktop/doesnt/exist/hopefully.desktop");
    }
  g_test_trap_assert_stderr ("*Unable to remove favorite*");
  
  settings->UnReference ();
}
