#include "test_service_hud.h"

namespace unity
{
namespace service
{
namespace
{
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
}


Hud::Hud()
{
  auto object = std::make_shared<glib::DBusObject>(hud_interface, "com.canonical.hud");
  object->SetMethodsCallsHandler(sigc::mem_fun(this, &Hud::OnMethodCall));
  object->registered.connect([this] (std::string const&) {
    timeout_.reset(new glib::TimeoutSeconds(1));
    timeout_->Run([this] { EmitSignal(); return true; });
  });

  server_.AddObject(object, "/com/canonical/hud");
}

void Hud::EmitSignal()
{
  GVariant *query;
  int num_entries = 5;

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

  query = g_variant_builder_end(&ret_builder);

  server_.EmitSignal("com.canonical.hud", "UpdatedQuery", query);
}

GVariant* Hud::OnMethodCall(std::string const& method, GVariant *parameters)
{
  if (method == "StartQuery")
  {
    glib::String query;
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

    return g_variant_builder_end(&ret_builder);
  }
  else if (method == "ExecuteQuery")
  {
    GVariant * key = NULL;
    key = g_variant_get_child_value(parameters, 0);
    g_variant_unref(key);
  }
  else if (method == "CloseQuery")
  {
    GVariant * key = NULL;
    key = g_variant_get_child_value(parameters, 0);
    g_variant_unref(key);
  }

  return nullptr;
}

}
}

