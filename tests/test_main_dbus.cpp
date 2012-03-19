#include <gtest/gtest.h>
#include <gio/gio.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>

static bool wait_until_test_service_appears();
static void tell_service_to_exit();

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  g_type_init();
  
  nux::NuxInitialize (0);

  // We need the service to be ready before we are
  if (!wait_until_test_service_appears())
  {
    std::cerr << "FATAL: Unable to connect to test service";
    return EXIT_FAILURE;
  }

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  int ret = RUN_ALL_TESTS();

  tell_service_to_exit();

  return ret;
}

static bool wait_until_test_service_appears()
{
  bool have_name = false;
  bool timeout_reached = false;

  auto timeout_cb = [](gpointer data) -> gboolean
  {
    *(bool*)data = true;
    return FALSE;
  };

  auto callback = [](GDBusConnection * conn,
                     const char * name,
                     const char * name_owner,
                     gpointer user_data)
  {
    *(bool*)user_data = true;
  };

  g_bus_watch_name(G_BUS_TYPE_SESSION,
                   "com.canonical.Unity.Test",
                   G_BUS_NAME_WATCHER_FLAGS_NONE,
                   callback,
                   NULL,
                   &have_name,
                   NULL);
  g_timeout_add(10000, timeout_cb, &timeout_reached);

  while (!have_name && !timeout_reached)
    g_main_context_iteration(g_main_context_get_thread_default(), TRUE);

  return (have_name && !timeout_reached);
}

static void tell_service_to_exit()
{
  // Ask the service to exit
  GDBusConnection* connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
  g_dbus_connection_call_sync(connection,
                              "com.canonical.Unity.Test",
                              "/com/canonical/unity/test/controller",
                              "com.canonical.Unity.Test",
                              "Exit",
                              NULL,
                              NULL,
                              G_DBUS_CALL_FLAGS_NONE,
                              -1,
                              NULL, NULL);
  g_object_unref(connection);
}
