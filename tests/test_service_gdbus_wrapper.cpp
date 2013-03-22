#include "test_service_gdbus_wrapper.h"

namespace unity
{
namespace service
{
namespace
{
const char * gdbus_wrapper_interface =
"<?xml version='1.0' encoding='UTF-8'?>\n"
"<node name='/'>\n"
"        <interface name='com.canonical.gdbus_wrapper'>\n"
"<!-- Properties -->\n"
"                <!-- None -->\n"
"\n"
"<!-- Functions -->\n"
"                <method name='TestMethod'>\n"
"                        <!-- in -->\n"
"                        <arg type='s' name='query' direction='in' />\n"
"                        <!-- out -->\n"
"                        <arg type='s' name='target' direction='out' />\n"
"                </method>\n"
"\n"
"                <method name='SetReadOnlyProperty'>\n"
"                        <arg type='i' name='query' direction='in' />\n"
"                </method>\n"
"\n"
"<!-- Signals -->\n"
"                <signal name='TestSignal'>\n"
"                        <arg type='s' name='target' direction='out' />\n"
"                </signal>\n"
"\n"
"<!-- Properties -->\n"
"                <property name='ReadOnlyProperty' type='i' access='read'/>\n"
"                <property name='WriteOnlyProperty' type='i' access='write'/>\n"
"                <property name='ReadWriteProperty' type='i' access='readwrite'/>\n"
"\n"
"<!-- End of interesting stuff -->\n"
"\n"
"        </interface>\n"
"</node>\n"
;
}

GDBus::GDBus()
  : ro_property_(0)
{
  auto object = glib::DBusObjectBuilder::GetObjectsForIntrospection(gdbus_wrapper_interface).front();
  object->SetMethodsCallsHandler([this] (std::string const& method, GVariant *parameters) -> GVariant* {
    auto const& object = server_.GetObjects().front();

    if (method == "TestMethod")
    {
      glib::String query;
      g_variant_get(parameters, "(s)", &query);

      object->EmitSignal("TestSignal", g_variant_new("(s)", query.Value()));
      return g_variant_new("(s)", query.Value());
    }
    else if (method == "SetReadOnlyProperty")
    {
      g_variant_get(parameters, "(i)", &ro_property_);
      object->EmitPropertyChanged("ReadOnlyProperty");
    }

    return nullptr;
  });

  object->SetPropertyGetter([this] (std::string const& property) -> GVariant* {
    if (property == "ReadOnlyProperty")
      return g_variant_new("(i)", ro_property_);

    return nullptr;
  });

  server_.AddObject(object, "/com/canonical/gdbus_wrapper");
}

}
}
