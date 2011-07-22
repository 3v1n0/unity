#include <gtest/gtest.h>
#include <gio/gio.h>
#include <NuxCore/Logger.h>

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  g_type_init();
  g_thread_init(NULL);

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if your debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));
  
  int ret = RUN_ALL_TESTS();

  // Ask the service to exit
  GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
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

  return ret;
}
