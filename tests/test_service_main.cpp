#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

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

  auto loop = g_main_loop_new(NULL, FALSE);

  nux::NuxInitialize(0);
  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_TEST_LOG_SEVERITY"));

  glib::DBusServer controller("com.canonical.Unity.Test");
  controller.AddObjects(introspection_xml, "/com/canonical/unity/test/controller");
  auto obj = controller.GetObjects().front();
  obj->SetMethodsCallsHandler([loop] (std::string const& method, GVariant*) {
    if (method == "Exit")
      g_main_loop_quit(loop);

    return static_cast<GVariant*>(nullptr);
  });

  service::Hud hud;
  service::GDBus gdbus;
  service::Panel panel;
  service::Model model;
  service::Scope scope(scope_id ? scope_id: "testscope1");

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  if (scope_id) g_free(scope_id);

  return 0;
}
