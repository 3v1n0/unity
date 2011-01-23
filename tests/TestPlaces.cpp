/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "Nux/Nux.h"
#include "Nux/VLayout.h"
#include "Nux/HLayout.h"
#include "Nux/TextureArea.h"
#include "Nux/WindowThread.h"
#include "NuxGraphics/GraphicsEngine.h"
#include <gtk/gtk.h>
#include "Nux/ComboBoxComplex.h"
#include "../src/ubus-server.h"
#include "Nux/TableCtrl.h"
#include "PlacesView.h"
#include "UBusMessages.h"

#include "PlaceFactoryFile.h"
#include "Place.h"
#include "PlaceEntry.h"

class TestApp 
{
public:
  TestApp ()
  {   
    nux::VLayout *layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

    _combo = new nux::ComboBoxComplex (NUX_TRACKER_LOCATION);
    _combo->SetPopupWindowSize (300, 150);
    _combo->SetMinimumWidth (300);
    layout->AddView (_combo, 0, nux::eCenter, nux::eFix);

    PlacesView *view = new PlacesView ();
    view->SetMinMaxSize(1024, 600);
    layout->AddView(view, 1, nux::eCenter, nux::eFix);

    _factory = new PlaceFactoryFile ();
    PopulateEntries ();
    _factory->place_added.connect (sigc::mem_fun (this, &TestApp::OnPlaceAdded));

    layout->SetContentDistribution(nux::eStackCenter);
    nux::GetGraphicsThread()->SetLayout (layout);
  }

  ~TestApp ()
  {

  }

  void OnPlaceAdded (Place *place)
  {
    std::vector<PlaceEntry *> entries = place->GetEntries ();
    std::vector<PlaceEntry *>::iterator i;

    for (i = entries.begin (); i != entries.end (); ++i)
    {
      PlaceEntry *entry = static_cast<PlaceEntry *> (*i);
      _combo->AddItem (new nux::TableItem(entry->GetName ()));
    }
  }

  void PopulateEntries ()
  {
    std::vector<Place *> places = _factory->GetPlaces ();
    std::vector<Place *>::iterator it;

    for (it = places.begin (); it != places.end (); ++it)
    {
      Place *place = static_cast<Place *> (*it);
      std::vector<PlaceEntry *> entries = place->GetEntries ();
      std::vector<PlaceEntry *>::iterator i;

      for (i = entries.begin (); i != entries.end (); ++i)
      {
        PlaceEntry *entry = static_cast<PlaceEntry *> (*i);
       _combo->AddItem (new nux::TableItem(entry->GetName ()));
      }
    }
  }

  void OnComboChangedFoRealz ()
  {
    std::vector<Place *> places = _factory->GetPlaces ();
    std::vector<Place *>::iterator it;
    gchar *txt = NULL;

    // Find entry
    for (it = places.begin (); it != places.end (); ++it)
    {
      Place *place = static_cast<Place *> (*it);
      std::vector<PlaceEntry *> entries = place->GetEntries ();
      std::vector<PlaceEntry *>::iterator i;

      for (i = entries.begin (); i != entries.end (); ++i)
      {
        PlaceEntry *entry = static_cast<PlaceEntry *> (*i);

        if (g_strcmp0 (txt, entry->GetName ()) == 0)
        {

        }
      }
    }

    g_free (txt);
  }

  nux::ComboBoxComplex *_combo;
  PlaceFactoryFile     *_factory;
};


void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{

}

int main(int argc, char **argv)
{
  TestApp *app;

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  nux::NuxInitialize(0);

  nux::WindowThread* wt = nux::CreateGUIThread("Unity Places",
                                               1024, 600, 0, &ThreadWidgetInit, 0);
  app = new TestApp ();
  
  wt->Run(NULL);
  delete wt;
  return 0;
}

