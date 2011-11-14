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

#ifndef PLACES_TILE_H
#define PLACES_TILE_H

#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <Nux/VLayout.h>
#include <NuxImage/CairoGraphics.h>

#include "Introspectable.h"

namespace unity
{

class PlacesTile : public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PlacesTile, nux::View);
public:
  PlacesTile(NUX_FILE_LINE_PROTO, const void* id = NULL);
  ~PlacesTile();

  const void* GetId();

  sigc::signal<void, PlacesTile*> sigClick;

protected:
  virtual nux::Geometry GetHighlightGeometry();
  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type);

private:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void UpdateBackground();
  nux::BaseTexture* DrawHighlight(std::string const& texid, int width, int height);

private:
  const void* _id;
  nux::ObjectPtr<nux::BaseTexture> _hilight_background;
  nux::TextureLayer* _hilight_layer;

  void OnFocusChanged(nux::Area* label);
  void OnFocusActivated(nux::Area* label);
  int _last_width;
  int _last_height;

  sigc::connection con_obj;
};

}

#endif // PLACE_TILE_H
