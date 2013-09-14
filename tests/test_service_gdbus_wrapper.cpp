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
"                        <arg type='i' name='value' direction='in' />\n"
"                </method>\n"
"\n"
"                <method name='GetWriteOnlyProperty'>\n"
"                        <arg type='i' name='value' direction='out' />\n"
"                </method>\n"
"\n"
"<!-- Signals -->\n"
"                <signal name='TestSignal'>\n"
"                        <arg type='s' name='target' />\n"
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
  , rw_property_(0)
  , wo_property_(0)
{
  auto object = glib::DBusObjectBuilder::GetObjectsForIntrospection(gdbus_wrapper_interface).front();
  object->SetMethodsCallsHandler([this, object] (std::string const& method, GVariant *parameters) -> GVariant* {
    if (method == "TestMethod")
    {
      glib::String query;
      g_variant_get(parameters, "(s)", &query);

      object->EmitSignal("TestSignal", g_variant_new("(s)", query.Value()));
      return g_variant_new("(s)", query.Value());
    }
    else if (method == "SetReadOnlyProperty")
    {
      int new_value = 0;
      g_variant_get(parameters, "(i)", &new_value);

      if (new_value != ro_property_)
      {
        ro_property_ = new_value;
        object->EmitPropertyChanged("ReadOnlyProperty");
      }
    }
    else if (method == "GetWriteOnlyProperty")
    {
      return g_variant_new("(i)", wo_property_);
    }

    return nullptr;
  });

  object->SetPropertyGetter([this] (std::string const& property) -> GVariant* {
    if (property == "ReadOnlyProperty")
      return g_variant_new_int32(ro_property_);
    else if (property == "ReadWriteProperty")
      return g_variant_new_int32(rw_property_);

    return nullptr;
  });

  object->SetPropertySetter([this] (std::string const& property, GVariant* value) -> bool {
    if (property == "ReadWriteProperty")
    {
      g_variant_get(value, "i", &rw_property_);
      return true;
    }
    else if (property == "WriteOnlyProperty")
    {
      g_variant_get(value, "i", &wo_property_);
      return true;
    }

    return false;
  });

  server_.AddObject(object, "/com/canonical/gdbus_wrapper");
}

}
}
