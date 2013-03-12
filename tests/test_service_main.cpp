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


int main(int argc, char** argv)
{
#if G_ENCODE_VERSION (GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION) <= GLIB_VERSION_2_34
  g_type_init();
#endif
  auto loop = g_main_loop_new(NULL, FALSE);

  glib::DBusServer controller("com.canonical.Unity.Test");
  controller.AddObjects(introspection_xml, "/com/canonical/unity/test/controller");
  auto const& obj = controller.GetObjects().front();
  obj->SetMethodsCallsHandler([loop] (std::string const& method, GVariant*) {
    if (method == "Exit")
      g_main_loop_quit(loop);

    return static_cast<GVariant*>(nullptr);
  });

  service::Hud hud;
  service::GDBus gdbus;
  service::Panel panel;
  service::Model model;

  g_main_loop_run(loop);
  g_main_loop_unref(loop);

  return 0;
}
