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

#ifndef PLACESTILE_H
#define PLACESTILE_H

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <sigc++/sigc++.h>

#include <Nux/Nux.h>
#include <NuxBasewindow.h>
#include <NuxCore/Math/MathInc.h>

#include <sigc++/trackable.h>
#include <sigc++/signal.h>
#include <sigc++/functors/ptr_fun.h>
#include <sigc++/functors/mem_fun.h>


class PlacesTile : public Introspectable, public nux::InitallyUnownedObject, public sigc::trackable, public Nux::View
{
public:
  typedef enum 
  {
    STATE_DEFAULT,
    STATE_HOVER,
    STATE_PRESSED,
    STATE_ACTIVATED,
  } TileState;

  PlacesTile ();
  ~Placestile ();
 
  virtual void SetCaption (const gchar *caption);
  virtual const gchar* GetCaption ();
 
  // mainly for testing
  virtual void SetState (TileState state);
  virtual TileState GetState ();
  
  sigc::signal<void> sigClick;
  sigc::signal<void> sigToggled;
  sigc::signal<void, bool> sigStateChanged;
  
protected:
  virtual long ProcessEvent (Nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (Nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void DrawContent (Nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual void PostDraw (Nux::GraphicsEngine &GfxContext, bool force_draw);
  
  void RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags);
  void RecvClick (int x, int y, unsigned long button_flags, unsigned long key_flags);
  
  
private:
  TileState _state;
  gchar *_caption;
};

#endif // PLACETILE_H
