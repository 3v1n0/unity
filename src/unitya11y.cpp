/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Alejandro Piñeiro Iglesias <apinheiro@igalia.com>
 */

#include <glib.h>
#include <gio/gio.h>
#include <gmodule.h>
#include <stdio.h>

#include "unity-util-accessible.h"

static void
unity_a11y_restore_environment (void)
{
  g_unsetenv ("NO_AT_BRIDGE");
  g_unsetenv ("NO_GAIL");
}

static void
load_unity_atk_util ()
{
  g_type_class_unref (g_type_class_ref (UNITY_TYPE_UTIL_ACCESSIBLE));
}

#define INIT_METHOD "gnome_accessibility_module_init"
#define DESKTOP_SCHEMA "org.gnome.desktop.interface"
#define ACCESSIBILITY_ENABLED_KEY "accessibility"
#define AT_SPI_SCHEMA "org.a11y.atspi"
#define ATK_BRIDGE_LOCATION_KEY "atk-bridge-location"

static gboolean
should_enable_a11y (void)
{
  GSettings *desktop_settings = NULL;
  gboolean value = FALSE;

  desktop_settings = g_settings_new (DESKTOP_SCHEMA);
  value = g_settings_get_boolean (desktop_settings, ACCESSIBILITY_ENABLED_KEY);

  g_object_unref (desktop_settings);

  return value;
}

static gchar*
get_atk_bridge_path (void)
{
  GSettings *atspi_settings = NULL;
  char *value = NULL;
  const char * const *list_schemas = NULL;
  gboolean found = FALSE;
  int i = 0;

  /* we need to check if AT_SPI_SCHEMA is present as g_setting_new
     could abort if the schema is not here*/
  list_schemas = g_settings_list_schemas ();
  for (i = 0; list_schemas [i]; i++)
    {
      if (!g_strcmp0 (list_schemas[i], AT_SPI_SCHEMA))
        {
          found = TRUE;
          break;
        }
    }

  if (!found)
    return NULL;

  atspi_settings = g_settings_new (AT_SPI_SCHEMA);
  value = g_settings_get_string (atspi_settings, ATK_BRIDGE_LOCATION_KEY);

  g_object_unref (atspi_settings);

  return value;
}

static gboolean
a11y_invoke_module (const char *module_path)
{
  GModule    *handle;
  void      (*invoke_fn) (void);

  if (!module_path)
    {
      g_warning ("Accessibility: invalid module path (NULL)");

      return FALSE;
    }

  if (!(handle = g_module_open (module_path, (GModuleFlags)0)))
    {
      g_warning ("Accessibility: failed to load module '%s': '%s'",
                 module_path, g_module_error ());

      return FALSE;
    }

  if (!g_module_symbol (handle, INIT_METHOD, (gpointer *)&invoke_fn))
    {
      g_warning ("Accessibility: error library '%s' does not include "
                 "method '%s' required for accessibility support",
                 module_path, INIT_METHOD);
      g_module_close (handle);

      return FALSE;
    }

  invoke_fn ();

  return TRUE;
}

/********************************************************************************/
/*
 * In order to avoid the atk-bridge loading and the GAIL
 * initialization during the gtk_init, it is required to set some
 * environment vars.
 *
 */
void
unity_a11y_preset_environment (void)
{
  g_setenv ("NO_AT_BRIDGE", "1", TRUE);
  g_setenv ("NO_GAIL", "1", TRUE);
}

/*
 * It loads the atk-bridge if required. It checks:
 *  * If the proper gsettings keys are set
 *  * Loads the proper AtkUtil implementation
 */
void
unity_a11y_init (void)
{
  gchar *bridge_path = NULL;

  g_debug ("Unity accessibility initialization");

  unity_a11y_restore_environment ();

  if (!should_enable_a11y ())
    return;

  load_unity_atk_util ();

  bridge_path = get_atk_bridge_path ();

  if (a11y_invoke_module (bridge_path))
    {
      g_debug ("Unity accessibility started, using bridge on %s",
               bridge_path);
    }

  g_free (bridge_path);
}
