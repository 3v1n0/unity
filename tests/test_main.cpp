#include <gtest/gtest.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>

#include "logger_helper.h"

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  gtk_init(&argc, &argv);
  setlocale(LC_ALL, "C");

  nux::NuxInitialize(0);
  std::unique_ptr<nux::WindowThread> win_thread(nux::CreateNuxWindow("Tests",
                                                300, 200, nux::WINDOWSTYLE_NORMAL,
                                                NULL, false, NULL, NULL));

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  unity::helper::configure_logging("UNITY_TEST_LOG_SEVERITY");

  // StandaloneWindowManager brought in at link time.
  int ret = RUN_ALL_TESTS();

  return ret;
}

