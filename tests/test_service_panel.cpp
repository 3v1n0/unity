#include "test_service_panel.h"

namespace unity
{
namespace service
{
namespace
{
static const char * panel_interface =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/\">\n"
"    <interface name=\"com.canonical.Unity.Panel.Service\">\n"
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

void add_entry_id(GVariantBuilder *b)
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

void add_entry_id_2(GVariantBuilder *b)
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
}


Panel::Panel()
  : sync_return_mode_(0)
  , trigger_resync1_sent_(false)
{
  auto object = glib::DBusObjectBuilder::GetObjectsForIntrospection(panel_interface).front();
  object->SetMethodsCallsHandler(sigc::mem_fun(this, &Panel::OnMethodCall));

  server_.AddObject(object, "/com/canonical/Unity/Panel/Service");
}

GVariant* Panel::OnMethodCall(std::string const& method, GVariant *parameters)
{
  if (method == "Sync")
  {
    GVariantBuilder b;

    g_variant_builder_init (&b, G_VARIANT_TYPE ("(a(ssssbbusbbi))"));
    g_variant_builder_open (&b, G_VARIANT_TYPE ("a(ssssbbusbbi)"));

    if (sync_return_mode_ == 0)
    {
      add_entry_id(&b);
      add_entry_id_2(&b);
    }
    else if (sync_return_mode_ == 1)
    {
      add_entry_id_2(&b);
      add_entry_id(&b);
    }

    if (sync_return_mode_ == 1)
    {
      trigger_resync1_sent_ = true;
    }

    g_variant_builder_close (&b);
    return g_variant_builder_end (&b);
  }
  else if (method == "TriggerResync1")
  {
    sync_return_mode_ = 1;
    trigger_resync1_sent_ = false;

    server_.GetObjects().front()->EmitSignal("ReSync", g_variant_new("(s)", ""));
  }
  else if (method == "TriggerResync1Sent")
  {
    return g_variant_new("(b)", trigger_resync1_sent_ ? TRUE : FALSE);
  }

  return nullptr;
}

}
}
