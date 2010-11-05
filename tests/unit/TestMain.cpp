
#include "config.h"
#include <glib.h>
#include <glib-object.h>

void TestFavoriteStoreGSettingsCreateSuite (void);

int
main (int argc, char **argv)
{
  g_setenv ("GSETTINGS_SCHEMA_DIR", BUILDDIR"/settings/", TRUE);

  g_type_init ();
  g_thread_init (NULL);
  
  g_test_init (&argc, &argv, NULL);

  //Keep alphabetical please
  TestFavoriteStoreGSettingsCreateSuite ();

  return g_test_run ();
}
