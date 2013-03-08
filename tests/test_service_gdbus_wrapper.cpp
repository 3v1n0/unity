#include "test_service_gdbus_wrapper.h"

namespace unity
{
namespace service
{
namespace
{
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
}

GDBus::GDBus()
{
  auto object = glib::DBusObjectBuilder::GetObjectsForIntrospection(gdbus_wrapper_interface).front();
  object->SetMethodsCallsHandler([object] (std::string const& method, GVariant *parameters) {
    if (method == "TestMethod")
    {
      glib::String query;
      g_variant_get(parameters, "(s)", &query);

      object->EmitSignal("TestSignal", g_variant_new("(s)", query.Value()));
      return g_variant_new("(s)", query.Value());
    }

    return static_cast<GVariant*>(nullptr);
  });

  server_.AddObject(object, "/com/canonical/gdbus_wrapper");
}

}
}
