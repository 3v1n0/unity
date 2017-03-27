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
#include <upstart/upstart-dbus.h>

#define COMPIZ_CONFIG_PROFILE_ENV "COMPIZ_CONFIG_PROFILE"
#define COMPIZ_CONFIG_DEFAULT_PROFILE "ubuntu"

extern const CCSInterfaceTable ccsDefaultInterfaceTable;

static gchar *
get_ccs_profile_env_from_env_list (const gchar **env_vars)
{
  gchar *var = NULL;

  for (; env_vars && *env_vars; ++env_vars)
    {
      if (g_str_has_prefix (*env_vars, COMPIZ_CONFIG_PROFILE_ENV "="))
        {
          var = g_strdup (*env_vars + G_N_ELEMENTS (COMPIZ_CONFIG_PROFILE_ENV));
          break;
        }
    }

  return var;
}

static gchar *
get_ccs_profile_env_from_session_manager ()
{
  GDBusConnection *bus;
  GVariant *environment_prop, *environment_prop_list;
  const gchar **env_vars;
  gchar *profile;

  profile = NULL;
  bus = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  environment_prop = g_dbus_connection_call_sync (bus,
                                                  "org.freedesktop.systemd1",
                                                  "/org/freedesktop/systemd1",
                                                  "org.freedesktop.DBus.Properties",
                                                  "Get",
                                                  g_variant_new ("(ss)",
                                                                  "org.freedesktop.systemd1.Manager",
                                                                  "Environment"),
                                                  G_VARIANT_TYPE ("(v)"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  -1,
                                                  NULL,
                                                  NULL);

  if (!environment_prop)
    goto out;

  g_variant_get (environment_prop, "(v)", &environment_prop_list, NULL);
  env_vars = g_variant_get_strv (environment_prop_list, NULL);
  profile = get_ccs_profile_env_from_env_list (env_vars);

  g_clear_pointer (&environment_prop, g_variant_unref);
  g_clear_pointer (&environment_prop_list, g_variant_unref);

  if (!profile && g_getenv ("UPSTART_SESSION"))
    {
      const gchar * const * empty_array[] = {0};
      environment_prop_list = g_dbus_connection_call_sync (bus,
                                                           DBUS_SERVICE_UPSTART,
                                                           DBUS_PATH_UPSTART,
                                                           DBUS_INTERFACE_UPSTART,
                                                           "ListEnv",
                                                           g_variant_new("(@as)",
                                                             g_variant_new_strv (NULL, 0)),
                                                           NULL,
                                                           G_DBUS_CALL_FLAGS_NONE,
                                                           -1,
                                                           NULL,
                                                           NULL);
      if (!environment_prop_list)
        goto out;

      g_variant_get (environment_prop_list, "(^a&s)", &env_vars);
      profile = get_ccs_profile_env_from_env_list (env_vars);

      g_variant_unref (environment_prop_list);
    }

out:
  g_object_unref (bus);

  return profile;
}

static gboolean
is_compiz_profile_available (const gchar *profile)
{
  gboolean is_available;
  gchar *profile_path;

  profile_path = g_strdup_printf ("%s/compiz-1/compizconfig/%s.ini",
                                  g_get_user_config_dir (), profile);
  is_available = g_file_test (profile_path, G_FILE_TEST_EXISTS);
  g_free (profile_path);

  if (!is_available)
    {
      profile_path = g_strdup_printf ("/etc/compizconfig/%s.ini", profile);
      is_available = g_file_test (profile_path, G_FILE_TEST_EXISTS);
      g_free (profile_path);
    }

  return is_available;
}

static gboolean
set_compiz_profile (CCSContext *ccs_context, const gchar *profile_name)
{
  CCSPluginList plugins;
  const char *ccs_backend;

  ccs_backend = ccsGetBackend (ccs_context);

  ccsSetProfile (ccs_context, profile_name);
  ccsReadSettings (ccs_context);
  ccsWriteSettings (ccs_context);

  if (g_strcmp0 (ccs_backend, "gsettings") == 0)
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
  const gchar *profile, *current_profile, *ccs_profile_env;
  gchar *session_manager_ccs_profile;

  if (argc < 2)
    {
      g_warning ("You need to pass a valid profile as argument\n");
      return 1;
    }

  session_manager_ccs_profile = get_ccs_profile_env_from_session_manager ();

  if (session_manager_ccs_profile)
    {
      g_setenv (COMPIZ_CONFIG_PROFILE_ENV, session_manager_ccs_profile, TRUE);
      g_clear_pointer (&session_manager_ccs_profile, g_free);
    }
  else
    {
      ccs_profile_env = g_getenv (COMPIZ_CONFIG_PROFILE_ENV);

      if (!ccs_profile_env || ccs_profile_env[0] == '\0')
        {
          g_setenv (COMPIZ_CONFIG_PROFILE_ENV, COMPIZ_CONFIG_DEFAULT_PROFILE, TRUE);
        }
    }

  profile = argv[1];

  if (!is_compiz_profile_available (profile))
    {
      g_warning ("Compiz profile '%s' not found", profile);
      return 1;
    }

  context = ccsContextNew (0, &ccsDefaultInterfaceTable);

  if (!context)
    {
      g_warning ("Impossible to get Compiz config context\n");
      return -1;
    }

  g_debug ("Setting profile to '%s' (for environment '%s')",
           profile, g_getenv (COMPIZ_CONFIG_PROFILE_ENV));

  current_profile = ccsGetProfile (context);

  if (g_strcmp0 (current_profile, profile) == 0)
    {
      g_print("We're already using profile '%s', no need to switch\n", profile);
      return 0;
    }

  if (!set_compiz_profile (context, profile))
    {
      ccsFreeContext (context);
      return 1;
    }

  ccsFreeContext (context);

  g_print ("Switched to profile '%s' (for environment '%s')\n",
           profile, g_getenv (COMPIZ_CONFIG_PROFILE_ENV));

  /* This is for printing debug informations */
  context = ccsContextNew (0, &ccsDefaultInterfaceTable);
  ccsFreeContext (context);

  return 0;
}
