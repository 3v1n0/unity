#include <gtest/gtest.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <NuxCore/Logger.h>
#include <Nux/Nux.h>
#include <config.h>

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  // init XDG_DATA_DIRS before GTK to point to the local test-dir as 
  // the environment is only read once by glib and then cached
  const std::string LOCAL_DATA_DIR = BUILDDIR"/tests/data:/usr/share";
  g_setenv("XDG_DATA_DIRS", LOCAL_DATA_DIR.c_str(), TRUE);


  gtk_init(&argc, &argv);
  setlocale(LC_ALL, "C");

  nux::NuxInitialize(0);
  std::unique_ptr<nux::WindowThread> win_thread(nux::CreateNuxWindow("Tests",
                                                300, 200, nux::WINDOWSTYLE_NORMAL,
                                                NULL, false, NULL, NULL));

  // Slightly higher as we're more likely to test things we know will fail
  nux::logging::configure_logging("<root>=error");

  // but you can still change it if you're debugging ;)
  nux::logging::configure_logging(::getenv("UNITY_LOG_SEVERITY"));

  // StandaloneWindowManager brought in at link time.
  int ret = RUN_ALL_TESTS();

  return ret;
}

