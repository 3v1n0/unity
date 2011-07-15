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
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#ifndef PLACES_HORIZONTAL_TILE_H
#define PLACES_HORIZONTAL_TILE_H

#include <sigc++/sigc++.h>

#include "IconTexture.h"
#include "Introspectable.h"
#include "PlacesTile.h"
#include "StaticCairoText.h"

class PlacesHorizontalTile : public unity::Introspectable, public PlacesTile
{
public:

  PlacesHorizontalTile (const char *icon,
                        const char *label,
                        const char *comment,
                        int         icon_size=64,
                        bool        defer_icon_loading=false,
                        const void *id=NULL);
  ~PlacesHorizontalTile ();

  void SetURI (const char *uri);

protected:
  nux::Geometry GetHighlightGeometry ();

  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);
  
  virtual void                    DndSourceDragBegin      ();
  virtual nux::NBitmapData *      DndSourceGetDragImage   ();
  virtual std::list<const char *> DndSourceGetDragTypes   ();
  virtual const char *            DndSourceGetDataForType (const char *type, int *size, int *format);
  virtual void                    DndSourceDragFinished   (nux::DndAction result);

private:
  nux::Geometry _highlight_geometry;
  char* _label;
  char* _comment;
  char* _icon;
  char* _uri;
  IconTexture *_icontex;
  nux::StaticCairoText *_cairotext;
};


#endif /* PLACES_HORIZONTAL_TILE_H */
