#include <gtest/gtest.h>
#include <gio/gio.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>
#include "test_utils.h"

#include "config.h"

static bool wait_until_test_service_appears();
static void tell_service_to_exit();
static void signal_handler(int sig);

static gboolean no_exit = FALSE;

static GOptionEntry entries[] =
{
  { "no-exit", 'n', 0, G_OPTION_ARG_NONE, &no_exit, "Do not handle exit call", NULL },
  { NULL }
};


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
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
  
  signal(SIGINT, signal_handler);
  nux::NuxInitialize (0);

  g_setenv("XDG_DATA_HOME", BUILDDIR"/tests/data", TRUE);

  // We need the service to be ready before we are

  if (!wait_until_test_service_appears())
  {
    std::cerr << "FATAL: Unable to connect to test service" << std::endl;
    return EXIT_FAILURE;
  }

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_TEST_LOG_SEVERITY"));

  int ret = RUN_ALL_TESTS();

  if (!no_exit)
    tell_service_to_exit();

  return ret;
}

static bool wait_until_test_service_appears()
{
  bool have_name = false;

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

  Utils::WaitUntil(have_name, 3);
  EXPECT_TRUE(have_name);

  return have_name;
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

static void signal_handler(int sig)
{
  tell_service_to_exit();
  exit(0);
}
