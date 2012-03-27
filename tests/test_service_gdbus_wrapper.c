#include "test_service_gdbus_wrapper.h"
#include <unity.h>
#include <gio/gio.h>

const char * gdbus_wrapper_interface = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/\">\n"
"        <interface name=\"com.canonical.gdbus_wrapper\">\n"
"<!-- Properties -->\n"
"                <!-- None -->\n"
"\n"
"<!-- Functions -->\n"
"                <method name=\"TestMethod\">\n"
"                        <!-- in -->\n"
"                        <arg type=\"s\" name=\"query\" direction=\"in\" />\n"
"                        <!-- out -->\n"
"                        <arg type=\"s\" name=\"target\" direction=\"out\" />\n"
"                </method>\n"
"\n"
"<!-- Signals -->\n"
"                <signal name=\"TestSignal\">\n"
"                        <arg type=\"s\" name=\"target\" direction=\"out\" />\n"
"                </signal>\n"
"\n"
"<!-- End of interesting stuff -->\n"
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

G_DEFINE_TYPE(ServiceGDBusWrapper, service_gdbus_wrapper, G_TYPE_OBJECT);
static GDBusNodeInfo * node_info = NULL;
static GDBusInterfaceInfo * iface_info = NULL;
static GDBusInterfaceVTable bus_vtable = {
  method_call: bus_method,
  get_property: NULL,
  set_property: NULL,
};


struct _ServiceGDBusWrapperPrivate
{
  GDBusConnection * bus;
  GCancellable * bus_lookup;
  guint bus_registration;
};

static void
service_gdbus_wrapper_dispose(GObject* object)
{
  ServiceGDBusWrapper* self = SERVICE_GDBUS_WRAPPER(object);
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

}

static void
service_gdbus_wrapper_class_init(ServiceGDBusWrapperClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_gdbus_wrapper_dispose;
  g_type_class_add_private (klass, sizeof (ServiceGDBusWrapperPrivate));

  if (node_info == NULL) 
  {
    GError * error = NULL;

    node_info = g_dbus_node_info_new_for_xml(gdbus_wrapper_interface, &error);
    if (error != NULL) 
    {
        g_error("Unable to parse GDBUS_WRAPPER interface: %s", error->message);
        g_error_free(error);
    }
  }

  if (node_info != NULL && iface_info == NULL) 
  {
    iface_info = g_dbus_node_info_lookup_interface(node_info,"com.canonical.gdbus_wrapper");
    if (iface_info == NULL) 
    {
      g_error("Unable to find interface 'com.canonical.gdbus_wrapper'");
    }
  }

}

static void
service_gdbus_wrapper_init(ServiceGDBusWrapper* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_GDBUS_WRAPPER, ServiceGDBusWrapperPrivate);
  self->priv->bus = NULL;
  self->priv->bus_lookup = NULL;
  self->priv->bus_registration = 0;
  
  self->priv->bus_lookup = g_cancellable_new();
  g_bus_get(G_BUS_TYPE_SESSION, self->priv->bus_lookup, bus_got_cb, self);
  
}

ServiceGDBusWrapper*
service_gdbus_wrapper_new()
{
  return g_object_new(SERVICE_TYPE_GDBUS_WRAPPER, NULL);
}

static void
bus_got_cb (GObject *object, GAsyncResult * res, gpointer user_data)
{
  GError * error = NULL;
  ServiceGDBusWrapper * self = SERVICE_GDBUS_WRAPPER(user_data);
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
                                                  /* path */       "/com/canonical/gdbus_wrapper",
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
bus_method (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{ 
  if (g_strcmp0(method_name, "TestMethod") == 0) 
  {
    GVariant * ret = NULL;
    GVariant * sig_data = NULL;
    GVariant * tmp = NULL;
    gchar * query = NULL;
    GError * error = NULL;
    g_variant_get(parameters, "(s)", &query);
    
    tmp = g_variant_new_string(query);
    ret = g_variant_new_tuple(&tmp, 1);
    g_dbus_method_invocation_return_value(invocation, ret);
    tmp = NULL; 
     
    // emit a signal with the same string as passed in every time this method is called
    
    tmp = g_variant_new_string(query);
    sig_data = g_variant_new_tuple(&tmp, 1);    
    g_dbus_connection_emit_signal(connection,
                                  NULL,                            /* destination bus, we don't care */
                                  "/com/canonical/gdbus_wrapper",  /* object path */
                                  "com.canonical.gdbus_wrapper",   /* interface name */
                                  "TestSignal",                    /* Signal name */
                                  sig_data,                        /* parameter */
                                  &error);                         /* error */
                                    
    if (error != NULL)
    {
      g_critical ("could not emit signal TestSignal with data %s", query);
      g_error_free(error);
    }

    g_free(query);
  }

  return;
}

