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

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "PlacesTile.h"

#include "Introspectable.h"

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

class IconTexture;
namespace nux
{
  class StaticCairoText;
}

class PlacesSimpleTile : public Introspectable, public PlacesTile
{
public:

  PlacesSimpleTile (const char *icon, const char *label);
  ~PlacesSimpleTile ();

  const char *      GetLabel ();
  const char *      GetIcon ();

protected:
  const gchar* GetName ();
  const gchar *GetChildsName ();
  void AddProperties (GVariantBuilder *builder);

private:
  const char* _label;
  const char* _icon;
  IconTexture *_icontex;
  nux::StaticCairoText *_cairotext;

};


#endif /* PLACES_SIMPLE_TILE_H */
