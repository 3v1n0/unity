#include "test_service_panel.h"
#include <unity.h>
#include <gio/gio.h>

static const char * panel_interface = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/\">\n"
"        <interface name=\"com.canonical.Unity.Panel.Service\">\n"
"\n"
"<!-- Begin of real methods/signals -->\n"
"    <method name='Sync'>"
"      <arg type='a(ssssbbusbbi)' name='state' direction='out'/>"
"    </method>"
"\n"
"    <signal name='ReSync'>"
"     <arg type='s' name='indicator_id' />"
"    </signal>"
"\n"
"<!-- Begin of test only methods/signals -->\n"
"\n"
"    <method name='TriggerResync1' />"
"\n"
"    <method name='TriggerResync1Sent'>"
"      <arg type='b' name='sent' direction='out'/>"
"    </method>"
"\n"
"        </interface>\n"
"</node>\n"
;
static void bus_got_cb          (GObject *object, GAsyncResult * res, gpointer user_data);
static void bus_method          (GDBusConnection *connection,
                                const gchar *sender,
                                const gchar *object_path,
                                const gchar *interface_name,
                                const gchar *method_name,
                                GVariant *parameters,
                                GDBusMethodInvocation *invocation,
                                gpointer user_data);

G_DEFINE_TYPE(ServicePanel, service_panel, G_TYPE_OBJECT);
static GDBusNodeInfo * node_info = NULL;
static GDBusInterfaceInfo * iface_info = NULL;
static GDBusInterfaceVTable bus_vtable = {
  method_call: bus_method,
  get_property: NULL,
  set_property: NULL,
};

struct _ServicePanelPrivate
{
  GDBusConnection * bus;
  GCancellable * bus_lookup;
  guint bus_registration;
  guint sig_emission_handle;
};

static void
service_panel_dispose(GObject* object)
{
  ServicePanel* self = SERVICE_PANEL(object);
  if (self->priv->bus_lookup != NULL) {
    g_cancellable_cancel(self->priv->bus_lookup);
    g_object_unref(self->priv->bus_lookup);
    self->priv->bus_lookup = NULL;
  }

  if (self->priv->bus_registration != 0) {
    g_dbus_connection_unregister_object(self->priv->bus, self->priv->bus_registration);
    self->priv->bus_registration = 0;
  }

  if (self->priv->bus != NULL) {
    g_object_unref(self->priv->bus);
    self->priv->bus = NULL;
  }

  if (self->priv->sig_emission_handle) {
    g_source_remove(self->priv->sig_emission_handle);
    self->priv->sig_emission_handle = 0;
  }

}

static void
service_panel_class_init(ServicePanelClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_panel_dispose;
  g_type_class_add_private (klass, sizeof (ServicePanelPrivate));

  if (node_info == NULL) 
  {
    GError * error = NULL;

    node_info = g_dbus_node_info_new_for_xml(panel_interface, &error);
    if (error != NULL) 
    {
        g_error("Unable to parse Panel interface: %s", error->message);
        g_error_free(error);
    }
  }

  if (node_info != NULL && iface_info == NULL) 
  {
    iface_info = g_dbus_node_info_lookup_interface(node_info,"com.canonical.Unity.Panel.Service");
    if (iface_info == NULL) 
    {
      g_error("Unable to find interface 'com.canonical.Unity.Panel.Service'");
    }
  }

}

static void
service_panel_init(ServicePanel* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_PANEL, ServicePanelPrivate);
  self->priv->bus = NULL;
  self->priv->bus_lookup = NULL;
  self->priv->bus_registration = 0;
  
  self->priv->bus_lookup = g_cancellable_new();
  g_bus_get(G_BUS_TYPE_SESSION, self->priv->bus_lookup, bus_got_cb, self);
}

ServicePanel*
service_panel_new()
{
  return g_object_new(SERVICE_TYPE_PANEL, NULL);
}

static void
bus_got_cb (GObject *object, GAsyncResult * res, gpointer user_data)
{
  GError * error = NULL;
  ServicePanel * self = SERVICE_PANEL(user_data);
  GDBusConnection * bus;

  bus = g_bus_get_finish(res, &error);
  if (error != NULL) {
    g_critical("Unable to get bus: %s", error->message);
    g_error_free(error);
    return;
  }

  self->priv->bus = bus;

  /* Register object */
  self->priv->bus_registration = g_dbus_connection_register_object(bus,
                                                  /* path */       "/com/canonical/Unity/Panel/Service",
                                                  /* interface */  iface_info,
                                                  /* vtable */     &bus_vtable,
                                                  /* userdata */   self,
                                                  /* destroy */    NULL,
                                                  /* error */      &error);

  if (error != NULL) {
    g_critical ("Unable to create bus connection object, %s", error->message);
    g_error_free(error);
    return;
  }

  return;
}

static void
add_entry_id(GVariantBuilder *b)
{
  g_variant_builder_add (b, "(ssssbbusbbi)",
                      "test_indicator_id",
                      "test_entry_id",
                      "test_entry_name_hint",
                      "test_entry_label",
                      TRUE, /* label sensitive */
                      TRUE, /* label visible */
                      0, /* image type */
                      "", /* image_data */
                      TRUE, /* image sensitive */
                      TRUE, /* image visible */
                      1 /* priority  */);
}

static void
add_entry_id_2(GVariantBuilder *b)
{
  g_variant_builder_add (b, "(ssssbbusbbi)",
                      "test_indicator_id",
                      "test_entry_id2",
                      "test_entry_name_hint2",
                      "test_entry_label2",
                      TRUE, /* label sensitive */
                      TRUE, /* label visible */
                      0, /* image type */
                      "", /* image_data */
                      TRUE, /* image sensitive */
                      TRUE, /* image visible */
                      1 /* priority  */);
}

static int sync_return_mode = 0;
static int trigger_resync1_sent = FALSE;

static void
bus_method (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
  if (g_strcmp0(method_name, "Sync") == 0) 
  {
    GVariantBuilder b;

    g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(ssssbbusbbi))"));
    g_variant_builder_open (&b, G_VARIANT_TYPE ("a(ssssbbusbbi)"));

    if (sync_return_mode == 0)
    {
      add_entry_id(&b);
      add_entry_id_2(&b);
    }
    else if (sync_return_mode == 1)
    {
      add_entry_id_2(&b);
      add_entry_id(&b);
    }

    g_variant_builder_close (&b);

    g_dbus_method_invocation_return_value(invocation, g_variant_builder_end (&b));

    if (sync_return_mode == 1)
    {
      trigger_resync1_sent = TRUE;
    }
  }
  else if (g_strcmp0(method_name, "TriggerResync1") == 0) 
  {
    sync_return_mode = 1;
    trigger_resync1_sent = FALSE;

    g_dbus_method_invocation_return_value(invocation, NULL);
    GVariantBuilder ret_builder;
    g_variant_builder_init(&ret_builder, G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(&ret_builder, g_variant_new_string(""));
    g_dbus_connection_emit_signal (connection, NULL, "/com/canonical/Unity/Panel/Service", "com.canonical.Unity.Panel.Service", "ReSync", g_variant_builder_end(&ret_builder), NULL);
  }
  else if (g_strcmp0(method_name, "TriggerResync1Sent") == 0) 
  {
    GVariantBuilder ret_builder;
    g_variant_builder_init(&ret_builder, G_VARIANT_TYPE ("(b)"));
    g_variant_builder_add_value (&ret_builder, g_variant_new_boolean(trigger_resync1_sent));
    g_dbus_method_invocation_return_value(invocation, g_variant_builder_end (&ret_builder));
  }

  return;
}

