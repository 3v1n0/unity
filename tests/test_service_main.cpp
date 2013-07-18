#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <unity.h>

#include "test_service_scope.h"
#include "test_service_model.h"
#include "test_service_hud.h"
#include "test_service_panel.h"
#include "test_service_gdbus_wrapper.h"

using namespace unity;

static const gchar introspection_xml[] =
  "<node>"
  "  <interface name='com.canonical.Unity.Test'>"
  ""
  "    <method name='Exit'>"
  "    </method>"
  ""
  "  </interface>"
  "</node>";

static gchar* scope_id = nullptr;

static GOptionEntry entries[] =
{
  { "scope-id", 's', 0, G_OPTION_ARG_STRING, &scope_id, "Test scope id (/com/canonical/unity/scope/SCOPE_NAME)", "SCOPE_NAME" },
  { NULL }
};


int main(int argc, char** argv)
{
#if G_ENCODE_VERSION (GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION) <= GLIB_VERSION_2_34
  g_type_init();
#endif

  GError *error = NULL;
  GOptionContext *context;
  context = g_option_context_new ("- DBus tests");
  g_option_context_add_main_entries (context, entries, NULL);
  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_print ("option parsing failed: %s\n", error->message);
    return 1;
  }

  nux::NuxInitialize(0);
  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_TEST_LOG_SEVERITY"));

  service::Hud hud;
  service::GDBus gdbus;
  service::Panel panel;
  service::Model model;
  service::Scope scope(scope_id ? scope_id: "testscope1");

  // all the services might have requested dbus names, let's consider
  // the controller name a "primary" name and we'll wait for it before running
  // the actual dbus tests (since the other names were requested before this
  // one they should be acquired before this one)
  glib::DBusServer controller("com.canonical.Unity.Test");
  controller.AddObjects(introspection_xml, "/com/canonical/unity/test/controller");
  auto obj = controller.GetObjects().front();
  obj->SetMethodsCallsHandler([] (std::string const& method, GVariant*) {
    if (method == "Exit")
      unity_scope_dbus_connector_quit();

    return static_cast<GVariant*>(nullptr);
  });

  // scope equivalent of running the main loop, needed as this also requests
  // a dbus name the scope uses
  unity_scope_dbus_connector_run();

  if (scope_id) g_free(scope_id);

  return 0;
}
