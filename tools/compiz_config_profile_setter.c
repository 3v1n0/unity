/*
 * Copyright (C) 2017 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include <ccs.h>
#include <gio/gio.h>

extern const CCSInterfaceTable ccsDefaultInterfaceTable;

static gboolean
is_compiz_profile_available (const gchar *profile)
{
  gboolean is_available;
  gchar *profile_path;

  profile_path = g_strdup_printf ("/etc/compizconfig/%s.ini", profile);
  is_available = g_file_test (profile_path, G_FILE_TEST_EXISTS);

  g_free (profile_path);

  return is_available;
}

static gboolean
set_compiz_profile (CCSContext *ccs_context, const gchar *profile_name)
{
  CCSPluginList plugins;
  const char *ccs_backend;
  const char *ccs_profile;

  ccs_profile = ccsGetProfile (ccs_context);

  if (g_strcmp0 (ccs_profile, profile_name) == 0)
    {
      return TRUE;
    }

  ccs_backend = ccsGetBackend (ccs_context);

  if (g_strcmp0 (ccs_backend, "gsettings") != 0)
    {
      g_warning ("Compiz GSettings backends different from GSettings aren't supported");
      return FALSE;
    }

  if (!is_compiz_profile_available (profile_name))
    {
      g_warning ("Compiz profile '%s' not found", profile_name);
      return FALSE;
    }

  ccsSetProfile (ccs_context, profile_name);
  ccsReadSettings (ccs_context);
  ccsWriteSettings (ccs_context);

  g_settings_sync ();

  plugins = ccsContextGetPlugins (ccs_context);

  for (CCSPluginList p = plugins; p; p = p->next)
    {
      CCSPlugin* plugin = p->data;
      ccsReadPluginSettings (plugin);
    }

  return TRUE;
}

int main(int argc, char *argv[])
{
  CCSContext *context;
  const gchar *profile;

  if (argc < 2)
    {
      g_warning ("You need to pass a valid profile as argument\n");
      return 1;
    }

  context = ccsContextNew (0, &ccsDefaultInterfaceTable);
  profile = argv[1];

  g_debug ("Setting profile to '%s'", profile);

  if (!context)
    return -1;

  if (!set_compiz_profile (context, profile))
    {
      ccsFreeContext (context);
      return 1;
    }

  ccsFreeContext (context);

  return 0;
}
