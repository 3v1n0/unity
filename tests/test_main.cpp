#include <gtest/gtest.h>
#include <glib-object.h>
#include <NuxCore/Logger.h>

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  g_type_init();
  g_thread_init(NULL);

  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  return RUN_ALL_TESTS();
}
