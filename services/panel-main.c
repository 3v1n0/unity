
#include <glib.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#include "panel-service.h"

static GDBusNodeInfo *introspection_data = NULL;


static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='org.myorg.MyObject'>"
  "    <method name='ChangeCount'>"
  "      <arg type='i' name='change' direction='in'/>"
  "    </method>"
  "    <property type='i' name='Count' access='read'/>"
  "    <property type='s' name='Name' access='readwrite'/>"
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

  if (g_strcmp0 (method_name, "ChangeCount") == 0)
    {
      gint change;
      g_variant_get (parameters, "(i)", &change);

      printf ("change: %d\n", change);

      g_dbus_method_invocation_return_value (invocation, NULL);
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
                                              "/com/canonical/Unity/Panel/Service",
                                              introspection_data->interfaces[0],
                                              &interface_vtable,
                                              service,
                                              NULL,
                                              NULL);
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
  g_debug ("%s", G_STRFUNC);
  gtk_main_quit ();
}

gint
main (gint argc, gchar **argv)
{
  PanelService *service;
  guint         owner_id;

  gtk_init (&argc, &argv);

  introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
  g_assert (introspection_data != NULL);

  service = panel_service_new ();

  owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                             "com.canonical.Unity.Panel.Service",
                             G_BUS_NAME_OWNER_FLAGS_NONE,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             service,
                             NULL);
  gtk_main ();

  g_bus_unown_name (owner_id);
  g_dbus_node_info_unref (introspection_data);
  g_object_unref (service);

  return 0;
}
