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
#include "Nux/ComboBoxSimple.h"
#include "../src/ubus-server.h"
#include "Nux/TableCtrl.h"
#include "PlacesView.h"
#include "UBusMessages.h"
#include "BGHash.h"

#include "PlaceFactoryFile.h"
#include "Place.h"
#include "PlaceEntry.h"

class TestApp
{
public:
  TestApp()
  {
    nux::VLayout* layout = new nux::VLayout(TEXT(""), NUX_TRACKER_LOCATION);

    _combo = new nux::ComboBoxSimple(NUX_TRACKER_LOCATION);
    _combo->SetMinimumWidth(150);
    _combo->sigTriggered.connect(sigc::mem_fun(this, &TestApp::OnComboChangedFoRealz));
    _combo->SetCanFocus(false);
    g_debug("can we focus? %s", _combo->CanFocus() ? "yes :(" : "no! :D");
    layout->AddView(_combo, 0, nux::eCenter, nux::eFix);

    _factory = PlaceFactory::GetDefault();
    PopulateEntries();
    _factory->place_added.connect(sigc::mem_fun(this, &TestApp::OnPlaceAdded));

    PlacesView* view = new PlacesView(_factory);
    view->SetMinMaxSize(1024, 768);
    layout->AddView(view, 1, nux::eCenter, nux::eFix);
    view->SetSizeMode(PlacesView::SIZE_MODE_HOVER);

    layout->SetContentDistribution(nux::eStackCenter);
    layout->SetFocused(true);
    nux::GetGraphicsThread()->SetLayout(layout);
  }

  ~TestApp()
  {

  }

  void OnPlaceAdded(Place* place)
  {
    std::vector<PlaceEntry*> entries = place->GetEntries();
    std::vector<PlaceEntry*>::iterator i;

    place->Connect();

    for (i = entries.begin(); i != entries.end(); ++i)
    {
      PlaceEntry* entry = static_cast<PlaceEntry*>(*i);
      _combo->AddItem(entry->GetName());
    }
  }

  void PopulateEntries()
  {
    std::vector<Place*> places = _factory->GetPlaces();
    std::vector<Place*>::iterator it;

    for (it = places.begin(); it != places.end(); ++it)
    {
      Place* place = static_cast<Place*>(*it);
      std::vector<PlaceEntry*> entries = place->GetEntries();
      std::vector<PlaceEntry*>::iterator i;

      place->Connect();

      for (i = entries.begin(); i != entries.end(); ++i)
      {
        PlaceEntry* entry = static_cast<PlaceEntry*>(*i);
        _combo->AddItem(entry->GetName());
      }
    }
  }

  void OnComboChangedFoRealz(nux::ComboBoxSimple* simple)
  {
    std::vector<Place*> places = _factory->GetPlaces();
    std::vector<Place*>::iterator it;
    const char* txt = _combo->GetSelectionLabel();

    // Find entry
    for (it = places.begin(); it != places.end(); ++it)
    {
      Place* place = static_cast<Place*>(*it);
      std::vector<PlaceEntry*> entries = place->GetEntries();
      std::vector<PlaceEntry*>::iterator i;

      for (i = entries.begin(); i != entries.end(); ++i)
      {
        PlaceEntry* entry = static_cast<PlaceEntry*>(*i);

        if (g_strcmp0(txt, entry->GetName()) == 0)
        {
          g_debug("Activated: %s", entry->GetName());
          ubus_server_send_message(ubus_server_get_default(),
                                   UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
                                   g_variant_new("(sus)",
                                                 entry->GetId(),
                                                 0,
                                                 ""));
        }
      }
    }
  }

  nux::ComboBoxSimple* _combo;
  PlaceFactory* _factory;
};


void ThreadWidgetInit(nux::NThread* thread, void* InitData)
{

}

int main(int argc, char** argv)
{
  g_type_init();
  g_thread_init(NULL);
  gtk_init(&argc, &argv);

  unity::BGHash bg_hash;

  nux::NuxInitialize(0);

  nux::WindowThread* wt = nux::CreateGUIThread("Unity Places",
                                               1024, 768, 0, &ThreadWidgetInit, 0);
  TestApp();

  wt->Run(NULL);
  delete wt;
  return 0;
}

