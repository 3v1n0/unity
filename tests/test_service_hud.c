#include "test_service_hud.h"
#include <unity.h>
#include <gio/gio.h>

const char * hud_interface = 
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/\">\n"
"        <interface name=\"com.canonical.hud\">\n"
"<!-- Properties -->\n"
"                <!-- None -->\n"
"\n"
"<!-- Functions -->\n"
"                <method name=\"StartQuery\">\n"
"                        <!-- in -->\n"
"                        <arg type=\"s\" name=\"query\" direction=\"in\" />\n"
"                        <arg type=\"i\" name=\"entries\" direction=\"in\" />\n"
"                        <!-- out -->\n"
"                        <arg type=\"s\" name=\"target\" direction=\"out\" />\n"
"                        <arg type=\"a(sssssv)\" name=\"suggestions\" direction=\"out\" />\n"
"                        <arg type=\"v\" name=\"querykey\" direction=\"out\" />\n"
"                </method>\n"
"\n"
"                <method name=\"ExecuteQuery\">\n"
"                        <arg type=\"v\" name=\"key\" direction=\"in\" />\n"
"                        <arg type=\"u\" name=\"timestamp\" direction=\"in\" />\n"
"                </method>\n"
"\n"
"                <method name=\"CloseQuery\">\n"
"                        <arg type=\"v\" name=\"querykey\" direction=\"in\" />\n"
"                </method>\n"
"\n"
"<!-- Signals -->\n"
"                <signal name=\"UpdatedQuery\">\n"
"                        <arg type=\"s\" name=\"target\" direction=\"out\" />\n"
"                        <arg type=\"a(sssssv)\" name=\"suggestions\" direction=\"out\" />\n"
"                        <arg type=\"v\" name=\"querykey\" direction=\"out\" />\n"
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

G_DEFINE_TYPE(ServiceHud, service_hud, G_TYPE_OBJECT);
static GDBusNodeInfo * node_info = NULL;
static GDBusInterfaceInfo * iface_info = NULL;
static GDBusInterfaceVTable bus_vtable = {
  method_call: bus_method,
  get_property: NULL,
  set_property: NULL,
};


struct _ServiceHudPrivate
{
  GDBusConnection * bus;
  GCancellable * bus_lookup;
  guint bus_registration;
};

static void
service_hud_dispose(GObject* object)
{
  ServiceHud* self = SERVICE_HUD(object);
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
service_hud_class_init(ServiceHudClass* klass)
{
  G_OBJECT_CLASS(klass)->dispose = service_hud_dispose;
  g_type_class_add_private (klass, sizeof (ServiceHudPrivate));

  if (node_info == NULL) 
  {
    GError * error = NULL;

    node_info = g_dbus_node_info_new_for_xml(hud_interface, &error);
    if (error != NULL) 
    {
        g_error("Unable to parse HUD interface: %s", error->message);
        g_error_free(error);
    }
  }

  if (node_info != NULL && iface_info == NULL) 
  {
    iface_info = g_dbus_node_info_lookup_interface(node_info,"com.canonical.hud");
    if (iface_info == NULL) 
    {
      g_error("Unable to find interface 'com.canonical.hud'");
    }
  }

}

static void
service_hud_init(ServiceHud* self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self, SERVICE_TYPE_HUD, ServiceHudPrivate);
  self->priv->bus = NULL;
  self->priv->bus_lookup = NULL;
  self->priv->bus_registration = 0;
  
  self->priv->bus_lookup = g_cancellable_new();
  g_bus_get(G_BUS_TYPE_SESSION, self->priv->bus_lookup, bus_got_cb, self);
  
}

ServiceHud*
service_hud_new()
{
  return g_object_new(SERVICE_TYPE_HUD, NULL);
}

static void
bus_got_cb (GObject *object, GAsyncResult * res, gpointer user_data)
{
  GError * error = NULL;
  ServiceHud * self = SERVICE_HUD(user_data);
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
                                                  /* path */       "/com/canonical/hud",
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
  if (g_strcmp0(method_name, "StartQuery") == 0) 
  {
    GVariant * ret = NULL;
    gchar * query = NULL;
    int num_entries = 0;

    g_variant_get(parameters, "(si)", &query, &num_entries);

    /* Build into into a variant */
    GVariantBuilder ret_builder;
    g_variant_builder_init(&ret_builder, G_VARIANT_TYPE_TUPLE);
    g_variant_builder_add_value(&ret_builder, g_variant_new_string("target"));
    GVariantBuilder builder;
    
    g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);
     
    int i = 0;
    for (i = 0; i < num_entries; i++) 
    {
      gchar* target = g_strdup_printf("test-%i", i);
      gchar* icon = g_strdup_printf("icon-%i", i);
      gchar* future_icon = g_strdup(icon);
      gchar* completion_text = g_strdup_printf("completion-%i", i);
      gchar* accelerator = g_strdup_printf("<alt>+whatever");

      GVariantBuilder tuple;
      g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
      g_variant_builder_add_value(&tuple, g_variant_new_string(target));
      g_variant_builder_add_value(&tuple, g_variant_new_string(icon));
      g_variant_builder_add_value(&tuple, g_variant_new_string(future_icon));
      g_variant_builder_add_value(&tuple, g_variant_new_string(completion_text));
      g_variant_builder_add_value(&tuple, g_variant_new_string(accelerator));
      // build a fake key
      GVariant* key;
      {
        GVariantBuilder keybuilder;
        g_variant_builder_init(&keybuilder, G_VARIANT_TYPE_TUPLE);
        g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
        g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
        g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
        g_variant_builder_add_value(&keybuilder, g_variant_new_int32(1986));

        key = g_variant_new_variant(g_variant_builder_end(&keybuilder));
      }
      g_variant_ref_sink(key);
      g_variant_builder_add_value(&tuple, key);
      g_variant_builder_add_value(&builder, g_variant_builder_end(&tuple));
      g_free(target);
      g_free(icon);
      g_free(future_icon);
      g_free(completion_text);
    }
    g_variant_builder_add_value(&ret_builder, g_variant_builder_end(&builder));
   
    GVariant* query_key;
    {
      GVariantBuilder keybuilder;
      g_variant_builder_init(&keybuilder, G_VARIANT_TYPE_TUPLE);
      g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
      g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
      g_variant_builder_add_value(&keybuilder, g_variant_new_string("dummy string"));
      g_variant_builder_add_value(&keybuilder, g_variant_new_int32(1986));

      query_key = g_variant_new_variant(g_variant_builder_end(&keybuilder));
    }
    g_variant_ref_sink(query_key);
    g_variant_builder_add_value(&ret_builder, query_key);
    
    ret = g_variant_builder_end(&ret_builder);

    g_dbus_method_invocation_return_value(invocation, ret);
    g_free(query);
  } 
  else if (g_strcmp0(method_name, "ExecuteQuery") == 0) 
  {
    GVariant * key = NULL;
    key = g_variant_get_child_value(parameters, 0);
    g_dbus_method_invocation_return_value(invocation, NULL);
    g_variant_unref(key);
  }
  else if (g_strcmp0(method_name, "CloseQuery") == 0)
  {
    GVariant * key = NULL;
    key = g_variant_get_child_value(parameters, 0);
    g_dbus_method_invocation_return_value(invocation, NULL);
    g_variant_unref(key);
  }

  return;
}

