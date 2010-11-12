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

#define BASE_STORE_FILE BUILDDIR"/settings/test-favorite-store-gsettings.store"
#define BASE_STORE_CONTENTS "[desktop/unity/launcher]\n" \
                            "favorites=['evolution.desktop', 'firefox.desktop']"

static const char *base_store_favs[] = { "evolution.desktop", "firefox.desktop", NULL };
static int         n_base_store_favs = 2;

static void TestAllocation   (void);
static void TestGetFavorites (void);

void
TestFavoriteStoreGSettingsCreateSuite ()
{
#define _DOMAIN "/Unit/FavoriteStoreGSettings"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
  g_test_add_func (_DOMAIN"/GetFavorites", TestGetFavorites);
}

static GSettingsBackend *
CreateDefaultKeyFileBackend ()
{
  GSettingsBackend *b;
  GError *error = NULL;

  g_file_set_contents (BASE_STORE_FILE,
                       BASE_STORE_CONTENTS,
                       -1,
                       &error);
  g_assert (error == NULL);

  b = g_keyfile_settings_backend_new (BASE_STORE_FILE, "/", "root");

  return b;
}

static void
TestAllocation ()
{
  FavoriteStoreGSettings *settings = new FavoriteStoreGSettings ();
  g_assert (settings != NULL);

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
      g_assert_cmpstr (basename, ==, base_store_favs[i]);

      g_free (basename);
    }

  settings->UnReference ();
}
