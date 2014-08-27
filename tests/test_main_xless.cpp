#include <gtest/gtest.h>
#include <gio/gio.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>
#include <config.h>
#include "test_utils.h"

const gchar* LOCAL_DATA_DIR = BUILDDIR"/tests/data:/usr/share";

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  setlocale(LC_ALL, "C");
  g_setenv("LC_ALL", "C", TRUE);
  Utils::init_gsettings_test_environment();
  g_setenv("XDG_DATA_DIRS", LOCAL_DATA_DIR, TRUE);

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  int ret = RUN_ALL_TESTS();

  return ret;
}

