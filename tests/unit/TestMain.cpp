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
#include <glib.h>
#include <glib-object.h>

void TestFavoriteStoreGSettingsCreateSuite (void);

int
main (int argc, char **argv)
{
  g_setenv ("GSETTINGS_SCHEMA_DIR", BUILDDIR"/settings/", TRUE);

  g_type_init ();
  g_thread_init (NULL);
  
  g_test_init (&argc, &argv, NULL);

  //Keep alphabetical please
  TestFavoriteStoreGSettingsCreateSuite ();

  return g_test_run ();
}
