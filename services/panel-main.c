// -*- Mode: C; tab-width:2; indent-tabs-mode: t; c-basic-offset: 2 -*-
/*
 * Copyright (C) 2010 Canonical Ltd
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *              Rodrigo Moya <rodrigo.moya@canonical.com>
 */

#include <glib.h>
#include <glib-unix.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <libido/libido.h>

#include "config.h"
#include "panel-a11y.h"
#include "panel-service.h"
#include "panel-service-private.h"

static GDBusNodeInfo *introspection_data = NULL;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='"UPS_IFACE"'>"
  ""
  "    <method name='Sync'>"
  "      <arg type='"ENTRY_ARRAY_SIGNATURE"' name='state' direction='out'/>"
  "    </method>"
  ""
  "    <method name='SyncOne'>"
  "      <arg type='s' name='indicator_id' direction='in'/>"
  "      <arg type='"ENTRY_ARRAY_SIGNATURE"' name='state' direction='out'/>"
  "    </method>"
  ""
  "    <method name='SyncGeometries'>"
  "      <arg type='s' name='panel_id' direction='in'/>"
  "      <arg type='a(siiii)' name='geometries' direction='in'/>"
  "    </method>"
  ""
  "    <method name='GetIconPaths'>"
  "      <arg type='as' name='paths' direction='out'/>"
  "    </method>"
  ""
  "    <method name='ShowEntry'>"
  "      <arg type='s' name='entry_id' direction='in'/>"
  "      <arg type='u' name='xid' direction='in'/>"
  "      <arg type='i' name='x' direction='in'/>"
  "      <arg type='i' name='y' direction='in'/>"
  "      <arg type='u' name='button' direction='in'/>"
  "    </method>"
  ""
  "    <method name='ShowEntriesDropdown'>"
  "      <arg type='as' name='entries_ids' direction='in'/>"
  "      <arg type='s' name='selected_entry' direction='in'/>"
  "      <arg type='u' name='xid' direction='in'/>"
  "      <arg type='i' name='x' direction='in'/>"
  "      <arg type='i' name='y' direction='in'/>"
  "    </method>"
  ""
  "    <method name='ShowAppMenu'>"
  "      <arg type='u' name='xid' direction='in'/>"
  "      <arg type='i' name='x' direction='in'/>"
  "      <arg type='i' name='y' direction='in'/>"
  "    </method>"
  ""
  "    <method name='SecondaryActivateEntry'>"
  "      <arg type='s' name='entry_id' direction='in'/>"
  "    </method>"
  ""
  "    <method name='ScrollEntry'>"
  "      <arg type='s' name='entry_id' direction='in'/>"
  "      <arg type='i' name='delta' direction='in'/>"
  "    </method>"
  ""
  ""
  "    <method name='CloseActiveEntry' />"
  ""
  "    <signal name='EntryActivated'>"
  "     <arg type='s' name='panel_id' />"
  "     <arg type='s' name='entry_id' />"
  "     <arg type='(iiuu)' name='entry_geometry' />"
  "    </signal>"
  ""
  "    <signal name='ReSync'>"
  "     <arg type='s' name='indicator_id' />"
  "    </signal>"
  ""
  "    <signal name='EntryActivateRequest'>"
  "     <arg type='s' name='entry_id' />"
  "    </signal>"
  ""
  "    <signal name='IconPathsChanged' />"
  ""
  "  </interface>"
  "</node>";


/* Forwards */
static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data);


static const GDBusInterfaceVTable interface_vtable =
{
  handle_method_call,
  NULL,
  NULL
};


/* Methods */

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
  PanelService *service = PANEL_SERVICE (user_data);

  if (g_strcmp0 (method_name, "Sync") == 0)
    {
      g_dbus_method_invocation_return_value (invocation,
                                             panel_service_sync (service));
    }
  else if (g_strcmp0 (method_name, "SyncOne") == 0)
    {
      gchar *id;
      g_variant_get (parameters, "(s)", &id, NULL);
      g_dbus_method_invocation_return_value (invocation,
                                             panel_service_sync_one (service, id));
      g_free (id);
    }
  else if (g_strcmp0 (method_name, "SyncGeometries") == 0)
    {
      GVariantIter *iter;
      gchar *panel_id, *entry_id;
      gint x, y, width, height;

      g_variant_get (parameters, "(sa(siiii))", &panel_id, &iter);

      if (panel_id)
        {
          if (iter)
            {
              while (g_variant_iter_loop (iter, "(siiii)", &entry_id, &x, &y,
                                                           &width, &height))
                {
                  panel_service_sync_geometry (service, panel_id, entry_id, x, y,
                                               width, height);
                }

              g_variant_iter_free (iter);
            }

          g_free (panel_id);
        }

      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_strcmp0 (method_name, "GetIconPaths") == 0)
    {
      gchar **paths;
      gint len;
      GVariant *ret_value;
      gtk_icon_theme_get_search_path (gtk_icon_theme_get_default (), &paths, &len);
      ret_value = g_variant_new("(@as)", g_variant_new_strv ((const gchar **) paths, len));
      g_dbus_method_invocation_return_value (invocation, ret_value);
      g_strfreev (paths);
    }
  else if (g_strcmp0 (method_name, "ShowEntry") == 0)
    {
      guint32 xid;
      gchar  *entry_id;
      gint32  x;
      gint32  y;
      guint32 button;
      g_variant_get (parameters, "(suiiu)", &entry_id, &xid, &x, &y, &button);

      panel_service_show_entry (service, entry_id, xid, x, y, button);

      g_dbus_method_invocation_return_value (invocation, NULL);
      g_free (entry_id);
    }
  else if (g_strcmp0 (method_name, "ShowEntriesDropdown") == 0)
    {
      guint32 xid;
      gchar **entries;
      gchar  *selected;
      gint32  x;
      gint32  y;
      g_variant_get (parameters, "(^assuii)", &entries, &selected, &xid, &x, &y);

      panel_service_show_entries (service, entries, selected, xid, x, y);

      g_dbus_method_invocation_return_value (invocation, NULL);
      g_strfreev (entries);
      g_free (selected);
    }
  else if (g_strcmp0 (method_name, "ShowAppMenu") == 0)
    {
      guint32 xid;
      gint32  x;
      gint32  y;
      g_variant_get (parameters, "(uii)", &xid, &x, &y);

      panel_service_show_app_menu (service, xid, x, y);

      g_dbus_method_invocation_return_value (invocation, NULL);
    }
  else if (g_strcmp0 (method_name, "SecondaryActivateEntry") == 0)
    {
      gchar *entry_id;
      g_variant_get (parameters, "(s)", &entry_id);

      panel_service_secondary_activate_entry (service, entry_id);

      g_dbus_method_invocation_return_value (invocation, NULL);
      g_free (entry_id);
    }
  else if (g_strcmp0 (method_name, "ScrollEntry") == 0)
    {
      gchar *entry_id;
      gint32 delta;
      g_variant_get (parameters, "(si)", &entry_id, &delta, NULL);
      panel_service_scroll_entry (service, entry_id, delta);
      g_dbus_method_invocation_return_value (invocation, NULL);
      g_free(entry_id);
    }
  else if (g_strcmp0 (method_name, "CloseActiveEntry") == 0)
    {
      panel_service_close_active_entry (service);
      g_dbus_method_invocation_return_value (invocation, NULL);
    }
}

static void
on_service_resync (PanelService *service, const gchar *indicator_id, GDBusConnection *connection)
{
  GError *error = NULL;
  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 UPS_PATH,
                                 UPS_IFACE,
                                 "ReSync",
                                 g_variant_new ("(s)", indicator_id),
                                 &error);
  if (error)
    {
      g_warning ("Unable to emit ReSync signal: %s", error->message);
      g_error_free (error);
    }
}

static void
on_service_entry_activated (PanelService    *service,
                            const gchar     *panel_id,
                            const gchar     *entry_id,
                            gint x, gint y, guint w, guint h,
                            GDBusConnection *connection)
{
  GError *error = NULL;
  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 UPS_PATH,
                                 UPS_IFACE,
                                 "EntryActivated",
                                 g_variant_new ("(ss(iiuu))",
                                                panel_id ? panel_id : "",
                                                entry_id ? entry_id : "",
                                                x, y, w, h),
                                 &error);

  if (error)
    {
      g_warning ("Unable to emit EntryActivated signal: %s", error->message);
      g_error_free (error);
    }
}

static void
on_service_entry_activate_request (PanelService    *service,
                                   const gchar     *entry_id,
                                   GDBusConnection *connection)
{
  GError *error = NULL;
  g_debug ("%s, entry_id: %s", G_STRFUNC, entry_id);

  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 UPS_PATH,
                                 UPS_IFACE,
                                 "EntryActivateRequest",
                                 g_variant_new ("(s)", entry_id),
                                 &error);

  if (error)
    {
      g_warning ("Unable to emit EntryActivateRequest signal: %s", error->message);
      g_error_free (error);
    }
}

static void
on_icon_theme_changed (GtkIconTheme* theme, GDBusConnection *connection)
{
  GError *error = NULL;
  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 UPS_PATH,
                                 UPS_IFACE,
                                 "IconPathsChanged",
                                 NULL,
                                 &error);

  if (error)
    {
      g_warning ("Unable to emit IconPathsChanged signal: %s", error->message);
      g_error_free (error);
    }
}

static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  PanelService *service = PANEL_SERVICE (user_data);
  guint         reg_id;

  reg_id = g_dbus_connection_register_object (connection,
                                              UPS_PATH,
                                              introspection_data->interfaces[0],
                                              &interface_vtable,
                                              service,
                                              NULL,
                                              NULL);
  g_signal_connect (service, "re-sync",
                    G_CALLBACK (on_service_resync), connection);
  g_signal_connect (service, "entry-activated",
                    G_CALLBACK (on_service_entry_activated), connection);
  g_signal_connect (service, "entry-activate-request",
                    G_CALLBACK (on_service_entry_activate_request), connection);

  g_signal_connect (gtk_icon_theme_get_default(), "changed",
                    G_CALLBACK (on_icon_theme_changed), connection);

  g_debug ("%s", G_STRFUNC);
  g_assert (reg_id > 0);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_debug ("Name Acquired");
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  PanelService *service = PANEL_SERVICE (user_data);

  g_debug ("%s", G_STRFUNC);
  if (service != NULL)
    g_signal_handlers_disconnect_by_data (service, connection);

  gtk_main_quit ();
}

static gboolean
on_unix_signal (gpointer data)
{
  PanelService *service = PANEL_SERVICE (data);
  g_signal_connect (service, "indicators-cleared",
                    G_CALLBACK (gtk_main_quit), NULL);
  panel_service_clear_indicators (service);
  return FALSE;
}

static void
discard_log_message (const gchar *log_domain, GLogLevelFlags log_level,
                     const gchar *message, gpointer user_data)
{
  return;
}

gint
main (gint argc, gchar **argv)
{
  PanelService *service;
  guint         owner_id;
  gboolean      lockscreen_mode = FALSE;
  GError *error = NULL;
  GOptionContext *context;
  GOptionEntry entries[] = {
    { "lockscreen-mode", 0, 0, G_OPTION_ARG_NONE, &lockscreen_mode, "Load indicators for the lockscreen", NULL },
    { NULL }};


  g_unsetenv ("UBUNTU_MENUPROXY");
  g_setenv ("NO_AT_BRIDGE", "1", TRUE);
  g_unsetenv ("NO_GAIL");

  gtk_init (&argc, &argv);
  gtk_icon_theme_append_search_path (gtk_icon_theme_get_default(), INDICATORICONDIR);
  ido_init ();

  context = g_option_context_new ("- Unity Panel Service");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  if (!g_option_context_parse (context, &argc, &argv, &error))
    {
      g_print ("unity-panel-service: %s\n", error->message);
      g_print ("Try --help for more information.\n");
      return 1;
    }

  panel_service_set_lockscreen_mode (lockscreen_mode);

  if (g_getenv ("SILENT_PANEL_SERVICE") != NULL)
  {
    g_log_set_default_handler (discard_log_message, NULL);
  }

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  panel_a11y_init ();

  service = panel_service_get_default ();

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             !lockscreen_mode ? UPS_NAME_DESKTOP : UPS_NAME_LOCKSCREEN,
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             service,
                             NULL);

  g_unix_signal_add (SIGINT, on_unix_signal, service);
  g_unix_signal_add (SIGTERM, on_unix_signal, service);

  gtk_main ();

  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (introspection_data);
  g_object_unref (service);

  return 0;
}
