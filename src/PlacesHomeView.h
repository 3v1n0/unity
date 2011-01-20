// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_HOME_VIEW_H
#define PLACES_HOME_VIEW_H

#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "Introspectable.h"

#include "Nux/EditTextBox.h"

class PlacesHomeView : public Introspectable, public nux::View
{
public:
  PlacesHomeView (NUX_FILE_LINE_PROTO);
  ~PlacesHomeView ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void PreLayoutManagement ();
  virtual long PostLayoutManagement (long LayoutResult);
  
protected:
  // Introspectable methods
  const gchar * GetName ();
  const gchar * GetChildsName (); 
  void AddProperties (GVariantBuilder *builder);

private:
  void UpdateBackground ();

private:
  nux::AbstractPaintLayer *_bg_layer;
  nux::HLayout            *_layout;
  int _last_width;
  int _last_height;
};


#endif
