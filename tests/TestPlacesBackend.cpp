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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */

#include "config.h"
#include <gtk/gtk.h>

#include "PlaceFactoryFile.h"
#include "Place.h"
#include "PlaceEntry.h"

class TestApp 
{
public:
  TestApp ()
  : _n_secs (0)
  {
    _window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_resize (GTK_WINDOW (_window), 250, 450);

    _vbox = gtk_vbox_new (FALSE, 12);
    gtk_container_add (GTK_CONTAINER (_window), _vbox);
      
    _combo = gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (_vbox), _combo, FALSE, FALSE, 0);
    g_signal_connect (_combo, "changed", G_CALLBACK (OnComboChanged), this);

    _seccombo= gtk_combo_box_text_new ();
    gtk_box_pack_start (GTK_BOX (_vbox), _seccombo, FALSE, FALSE, 0);
    
    _factory = new PlaceFactoryFile ();
    PopulateEntries ();
    _factory->place_added.connect (sigc::mem_fun (this, &TestApp::OnPlaceAdded));

    gtk_widget_show_all (_window);
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
      gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_combo), entry->GetName ());
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
        gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_combo), entry->GetName ());
      }
    }
  }

  static void OnComboChanged (GtkWidget *combo, TestApp *self)
  {
    self->OnComboChangedFoRealz ();
  }

  void OnComboChangedFoRealz ()
  {
    std::vector<Place *> places = _factory->GetPlaces ();
    std::vector<Place *>::iterator it;
    gchar *txt;
    
    txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (_combo));

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
          //FIXME : Update when we have sections support
#if 0
          DeeModel *sections = entry->GetSectionsModel ();
          DeeModelIter *iter;

          // Update the sections

          for (gint i = 0; i < _n_secs; i++)
          {
            gtk_combo_box_remove_text (GTK_COMBO_BOX (_seccombo), 0);
          }

          iter = dee_model_get_first_iter (sections);
          while (!dee_model_is_last(sections, iter))
          {
            gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (_seccombo), dee_model_get_string (sections, iter, 0));
            _n_secs++;

            iter = dee_model_next (sections, iter);
          }

          gtk_combo_box_set_active (GTK_COMBO_BOX (_seccombo), 0);
#endif
        }
      }
    }

    g_free (txt);
  }

  PlaceFactoryFile *_factory;

  GtkWidget *_window;
  GtkWidget *_vbox;
  GtkWidget *_combo;
  GtkWidget *_seccombo;
  gint       _n_secs;
};


int
main (int argc, char **argv)
{
  TestApp *app;

  g_type_init ();
  g_thread_init (NULL);
  gtk_init (&argc, &argv);

  app = new TestApp ();

  gtk_main ();

  delete app;

  return 0;
}
