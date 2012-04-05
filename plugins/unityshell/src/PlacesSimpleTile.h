// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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

#ifndef PLACES_SIMPLE_TILE_H
#define PLACES_SIMPLE_TILE_H

#include <sigc++/sigc++.h>

#include "IconTexture.h"
#include "Introspectable.h"
#include "PlacesTile.h"
#include "StaticCairoText.h"

namespace unity
{

class PlacesSimpleTile : public unity::debug::Introspectable, public PlacesTile
{
  NUX_DECLARE_OBJECT_TYPE(PlacesSimpleTile, PlacesTile);
public:

  PlacesSimpleTile(std::string const& icon, std::string const& label, int icon_size = 64, bool defer_icon_loading = false, const void* id = NULL);

  std::string GetLabel() const;
  std::string GetIcon() const;
  std::string GetURI() const;
  void SetURI(std::string const& uri);

  void LoadIcon();

protected:
  nux::Geometry GetHighlightGeometry();

  std::string GetName() const;
  void        AddProperties(GVariantBuilder* builder);

  virtual bool                    DndSourceDragBegin();
  virtual nux::NBitmapData*       DndSourceGetDragImage();
  virtual std::list<const char*> DndSourceGetDragTypes();
  virtual const char*             DndSourceGetDataForType(const char* type, int* size, int* format);
  virtual void                    DndSourceDragFinished(nux::DndAction result);

private:
  nux::Geometry _highlight_geometry;
  std::string _label;
  std::string _icon;
  std::string _uri;
  int   _idealiconsize;
  IconTexture* _icontex;
  nux::StaticCairoText* _cairotext;
};

}

#endif /* PLACES_SIMPLE_TILE_H */
