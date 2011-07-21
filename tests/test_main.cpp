#include <gtest/gtest.h>
#include <glib-object.h>
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

  return RUN_ALL_TESTS();
}
