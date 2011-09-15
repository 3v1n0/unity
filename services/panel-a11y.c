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
 * Authored by: Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#include <gio/gio.h>

#include "panel-a11y.h"
#include "panel-util-accessible.h"

static gboolean a11y_initialized = FALSE;

#define INIT_METHOD "gnome_accessibility_module_init"
#define DESKTOP_SCHEMA "org.gnome.desktop.interface"
#define ACCESSIBILITY_ENABLED_KEY "toolkit-accessibility"
#define AT_SPI_SCHEMA "org.a11y.atspi"
#define ATK_BRIDGE_LOCATION_KEY "atk-bridge-location"


/* This method is required because g_setting_new abort if the schema
   is not present. */
static gboolean
has_gsettings_schema (const gchar *schema)
{
  const char * const *list_schemas = NULL;
  gboolean found = FALSE;
  int i = 0;

  list_schemas = g_settings_list_schemas ();
  for (i = 0; list_schemas [i]; i++)
    {
      if (!g_strcmp0 (list_schemas[i], schema))
        {
          found = TRUE;
          break;
        }
    }

  return found;
}

static gboolean
should_enable_a11y (void)
{
  GSettings *desktop_settings = NULL;
  gboolean value = FALSE;

  if (!has_gsettings_schema (DESKTOP_SCHEMA))
    return FALSE;

  desktop_settings = g_settings_new (DESKTOP_SCHEMA);
  value = g_settings_get_boolean (desktop_settings, ACCESSIBILITY_ENABLED_KEY);

  g_object_unref (desktop_settings);

  return value;
}

static gchar*
get_atk_bridge_path (void)
{
  GSettings *atspi_settings = NULL;
  GVariant *variant = NULL;
  char *value = NULL;

  if (!has_gsettings_schema (AT_SPI_SCHEMA))
    return NULL;

  atspi_settings = g_settings_new(AT_SPI_SCHEMA);
  variant = g_settings_get_value (atspi_settings, ATK_BRIDGE_LOCATION_KEY);
  value = g_variant_dup_bytestring (variant, NULL);

  g_variant_unref (variant);
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

void
panel_a11y_init (void)
{
  gchar *bridge_path = NULL;

  if (a11y_initialized)
    return;

  /* Restore environment to load AT bridge */
  g_unsetenv ("NO_AT_BRIDGE");
  g_unsetenv ("NO_GAIL");

  if (!should_enable_a11y ())
    return;

  /* Load PanelUtilAccessible class */
  g_type_class_unref (g_type_class_ref (PANEL_TYPE_UTIL_ACCESSIBLE));

  bridge_path = get_atk_bridge_path ();
  if (a11y_invoke_module (bridge_path))
    {
      g_debug ("Unity accessibility started, using bridge on %s", bridge_path);
    }

  g_free (bridge_path);
  atk_get_root ();

  a11y_initialized = TRUE;
}
