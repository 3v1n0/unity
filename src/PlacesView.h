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
#include "NuxGraphics/GraphicsEngine.h"
#include "Nux/AbstractPaintLayer.h"
#include <Nux/BaseWindow.h>

#include "Introspectable.h"

class PlacesView : public Introspectable, public nux::BaseWindow
{
public:
  PlacesView (NUX_FILE_LINE_PROTO);
  ~PlacesView ();

  virtual long ProcessEvent(nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void ShowWindow (bool b, bool start_modal = false);

  void Show ();
  void Hide ();

  bool IsVisible;

protected:

  const gchar* GetName ();
  void AddProperties (GVariantBuilder *builder);
};

#endif // PANEL_HOME_BUTTON_H

