#include <gtest/gtest.h>
#include <gio/gio.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
#if G_ENCODE_VERSION (GLIB_MAJOR_VERSION, GLIB_MINOR_VERSION) <= GLIB_VERSION_2_34
  g_type_init();
#endif
  setlocale(LC_ALL, "C");

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  int ret = RUN_ALL_TESTS();

  return ret;
}

