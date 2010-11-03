#include <stdlib.h>

#include "FavoriteStore.h"
#include "FavoriteStoreGSettings.h"

static void TestAllocation (void);

void
TestFavoriteStoreGSettingsCreateSuite ()
{
#define _DOMAIN "/Unit/FavoriteStoreGSettings"

  g_test_add_func (_DOMAIN"/Allocation", TestAllocation);
}

// Not a real test, obviously
static void
TestAllocation ()
{
  FavoriteStoreGSettings *settings;

  settings = new FavoriteStoreGSettings ();

  GSList *favs = settings->GetFavorites ();
  GSList *f;
  for (f = favs; f; f = f->next)
    g_print ("%s\n", (char*)f->data);

  //settings->AddFavorite    ("/usr/share/applications/gnome-system-monitor.desktop", 1);
  //settings->RemoveFavorite ("/usr/share/applications/gnome-system-monitor.desktop");

  settings->UnReference ();
}
