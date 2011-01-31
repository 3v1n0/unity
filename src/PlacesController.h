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

#ifndef PLACES_CONTROLLER_H
#define PLACES_CONTROLLER_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include "Nux/Layout.h"
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "PlaceFactoryFile.h"
#include "PlacesView.h"
#include "Introspectable.h"

#include <Nux/BaseWindow.h>

class PlacesController : public Introspectable
{
public:
  PlacesController ();
  ~PlacesController ();

  void Show ();
  void Hide ();
  void ToggleShowHide ();

protected:
  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);

  static void ExternalActivation (GVariant *data, void *val);
  static void CloseRequest (GVariant *data, void *val);
  static void WindowConfigureCallback(int WindowWidth, int WindowHeight,
                                      nux::Geometry& geo, void *user_data);

  void RecvMouseDownOutsideOfView (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void OnActivePlaceEntryChanged (PlaceEntry *entry);

private:
  nux::BaseWindow  *_window;
  nux::HLayout     *_window_layout;
  PlacesView       *_view;
  PlaceFactoryFile *_factory;
  bool              _visible;
};

#endif // PLACES_CONTROLLER_H
