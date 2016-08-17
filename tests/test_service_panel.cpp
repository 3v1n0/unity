#include "test_service_panel.h"
#include "panel-service-private.h"

namespace unity
{
namespace service
{
namespace
{
static const char * panel_interface =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<node name=\"/\">\n"
"    <interface name=\"" UPS_IFACE "\">\n"
"\n"
"<!-- Begin of real methods/signals -->\n"
"    <method name='Sync'>"
"      <arg type='" ENTRY_ARRAY_SIGNATURE "' name='state' direction='out'/>"
"    </method>"
"\n"
"    <method name='GetIconPaths'>"
"      <arg type='as' name='paths' direction='out'/>"
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

void add_entry_id(GVariantBuilder *b, gint priority)
{
  g_variant_builder_add (b, ENTRY_SIGNATURE,
                         "test_indicator_id",
                         "test_entry_id",
                         "test_entry_name_hint",
                         0, /* parent window */
                         "test_entry_label",
                         TRUE, /* label sensitive */
                         TRUE, /* label visible */
                         0, /* image type */
                         "", /* image_data */
                         TRUE, /* image sensitive */
                         TRUE, /* image visible */
                         priority);
}

void add_entry_id_2(GVariantBuilder *b, gint priority)
{
  g_variant_builder_add (b, ENTRY_SIGNATURE,
                         "test_indicator_id",
                         "test_entry_id2",
                         "test_entry_name_hint2",
                         12345, /* parent window */
                         "test_entry_label2",
                         TRUE, /* label sensitive */
                         TRUE, /* label visible */
                         0, /* image type */
                         "", /* image_data */
                         TRUE, /* image sensitive */
                         TRUE, /* image visible */
                         priority);
}
}


Panel::Panel()
  : sync_return_mode_(0)
  , trigger_resync1_sent_(false)
{
  auto object = glib::DBusObjectBuilder::GetObjectsForIntrospection(panel_interface).front();
  object->SetMethodsCallsHandler(sigc::mem_fun(this, &Panel::OnMethodCall));

  server_.AddObject(object, UPS_PATH);
}

GVariant* Panel::OnMethodCall(std::string const& method, GVariant *parameters)
{
  if (method == "Sync")
  {
    GVariantBuilder b;

    g_variant_builder_init (&b, G_VARIANT_TYPE ("(" ENTRY_ARRAY_SIGNATURE ")"));
    g_variant_builder_open (&b, G_VARIANT_TYPE (ENTRY_ARRAY_SIGNATURE));

    if (sync_return_mode_ == 0)
    {
      add_entry_id(&b, 1);
      add_entry_id_2(&b, 2);
    }
    else if (sync_return_mode_ == 1)
    {
      add_entry_id_2(&b, 1);
      add_entry_id(&b, 2);
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
  else if (method == "GetIconPaths")
  {
    return g_variant_new("(as)", nullptr);
  }

  return nullptr;
}

}
}
