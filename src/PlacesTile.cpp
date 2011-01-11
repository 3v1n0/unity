/*
 * Copyright 2011 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Gordon Allott <gord.allott@canonical.com>
 *
 */

#include <nux-0.9/Nux.h>
#include "Placestile.h"

  PlacesTile::PlacesTile ()
  {
    _state = STATE_DEFAULT;

    OnMouseClick.connect (sigc::mem_fun (this, &PlacesTile::RecvClick) );
    OnMouseDown.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseDown) );
    OnMouseDoubleClick.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseDown) );
    OnMouseUp.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseUp) );
    OnMouseMove.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseMove) );
    OnMouseEnter.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseEnter) );
    OnMouseLeave.connect (sigc::mem_fun (this, &PlacesTile::RecvMouseLeave) );
  
    SetMinimumSize (60, 60);

    SetTextColor (Color::White);
  }

  long PlacesTile::ProcessEvent (Nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo)
  {
    long ret = TraverseInfo;
    ret = PostProcessEvent2 (ievent, ret, ProcessEventInfo);
    return ret;
  }

  void PlacesTile::Draw (Nux::GraphicsEngine &GfxContext, bool force_draw)
  {
    Geometry base = Nux::GetGeometry ();
  
    Nux::GetPainter ().PushDrawSlicedTextureLayer (GfxContext, base, eBUTTON_FOCUS, Nux::Color:White, Nux::eAllCorners);  
    
  }

  void PlacesTile::DrawContent (Nux::GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void PlacesTile::PostDraw (Nux::GraphicsEngine &GfxContext, bool force_draw)
  {

  }

  void PlacesTile::SetCaption (const gchar *caption)
  {
    _caption = g_strdup (caption);
    NeedRedraw ();
  }

  const gchar *GetCaption ()
  {
    return g_strdup (_caption);
  }

  void PlacesTile::SetState (TileState state)
  {
    _state = state;
  }

  TileState PlacseTile::GetState ()
  {
    return state;
  }

  void PlacesTile::RecvClick (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    sigClick.emit();
    NeedRedraw();
  }

  void PlacesTile::RecvMouseUp (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    NeedRedraw();
  }

  void PlacesTile::RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    _state = STATE_PRESSED;
    NeedRedraw();
  }

  void PlacesTile::RecvMouseMove (int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
  {

  }

  void PlacesTile::RecvMouseEnter (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    NeedRedraw();
  }

  void PlacesTile::RecvMouseLeave (int x, int y, unsigned long button_flags, unsigned long key_flags)
  {
    NeedRedraw();
  }

