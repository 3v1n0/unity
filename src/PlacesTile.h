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
#include <Nux/BaseWindow.h>
#include <NuxCore/Math/MathInc.h>

#include "StaticCairoText.h"

#include "Introspectable.h"

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>

class PlacesTile : public nux::View
{
public:
  typedef enum
  {
    STATE_DEFAULT,
    STATE_HOVER,
    STATE_PRESSED,
    STATE_ACTIVATED,
  } TileState;

  PlacesTile (NUX_FILE_LINE_PROTO);
  ~PlacesTile ();

  // mainly for testing
  virtual void SetState (TileState state);
  virtual TileState GetState ();

  sigc::signal<void> sigClick;
  sigc::signal<void> sigToggled;
  sigc::signal<void, bool> sigStateChanged;

  sigc::signal<void, int> MouseDown;
  sigc::signal<void, int> MouseUp;
  sigc::signal<void>      MouseEnter;
  sigc::signal<void>      MouseLeave;
  sigc::signal<void, int> MouseClick;

protected:

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void PostDraw (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void PreLayoutManagement ();
  virtual long PostLayoutManagement (long LayoutResult);


  void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  //void RecvMouseDrag (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);

  sigc::signal<void, PlacesTile*> sigMouseEnter;
  sigc::signal<void, PlacesTile*> sigMouseLeave;
  sigc::signal<void, PlacesTile*, int, int> sigMouseReleased;
  sigc::signal<void, PlacesTile*, int, int> sigMouseClick;
  sigc::signal<void, PlacesTile*, int, int> sigMouseDrag;

  TileState _state;
  nux::Layout *_layout;
  nux::BaseTexture *_hilight_background;

  void UpdateBackground ();
  void DrawRoundedRectangle (cairo_t* cr,
                             double   aspect,
                             double   x,
                             double   y,
                             double   cornerRadius,
                             double   width,
                             double   height);

};

#endif // PLACE_TILE_H
