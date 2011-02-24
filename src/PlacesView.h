/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 */

#ifndef PLACES_VIEW_H
#define PLACES_VIEW_H

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <Nux/AbstractPaintLayer.h>
#include <Nux/VLayout.h>
#include <Nux/LayeredLayout.h>

#include "Introspectable.h"

#include "Place.h"
#include "PlaceEntry.h"
#include "PlaceEntryHome.h"
#include "PlaceFactory.h"

#include "PlacesSearchBar.h"
#include "PlacesHomeView.h"

#include "PlacesSimpleTile.h"
#include "PlacesGroup.h"
#include "PlacesResultsController.h"
#include "PlacesResultsView.h"

#include "IconLoader.h"

class PlacesView : public nux::View, public Introspectable
{
  NUX_DECLARE_OBJECT_TYPE (PlacesView, nux::View);
public:
  PlacesView (PlaceFactory *factory);
  ~PlacesView ();

  // Return the TextEntry View. This is required to enable the keyboard focus on the text entry when the
  // dahs is shown.
  nux::TextEntry* GetTextEntryView ();

  // nux::View overrides
  long ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  // Methods
  void         SetActiveEntry (PlaceEntry *entry,
                               guint       section_id,
                               const char *search_string,
                               bool        signal=true);

  PlaceEntry * GetActiveEntry ();
  
  PlacesResultsController * GetResultsController ();


  // UBus handlers
  void PlaceEntryActivateRequest (const char *entry_id, guint section, const gchar *search);

  // Signals
  sigc::signal<void, PlaceEntry *> entry_changed;
 
protected:

  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

private:
  static void CloseRequest (GVariant *data, PlacesView *self);

  static void OnGroupAdded    (DeeModel *model, DeeModelIter *iter, PlacesView *self);
  static void OnGroupRemoved  (DeeModel *model, DeeModelIter *iter, PlacesView *self);
  static void OnResultAdded   (DeeModel *model, DeeModelIter *iter, PlacesView *self);
  static void OnResultRemoved (DeeModel *model, DeeModelIter *iter, PlacesView *self);

  void OnResultClicked (PlacesTile *tile);
  void OnSearchChanged (const char *search_string);

private:
  PlaceFactory       *_factory;
  nux::VLayout       *_layout;
  nux::LayeredLayout *_layered_layout;
  PlacesSearchBar    *_search_bar;
  PlacesHomeView     *_home_view;
  PlaceEntryHome     *_home_entry;
  PlaceEntry         *_entry;
  gulong              _group_added_id;
  gulong              _group_removed_id;
  gulong              _result_added_id;
  gulong              _result_removed_id;

  PlacesResultsController *_results_controller;
  PlacesResultsView       *_results_view;

  IconLoader     *_icon_loader;
};

#endif // PANEL_HOME_BUTTON_H

