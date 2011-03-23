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
 * Authored by: Mirco Müller <mirco.mueller@canonical.com>
 */

#ifndef PLACES_VSCROLLBAR_H
#define PLACES_VSCROLLBAR_H

#include "Nux/Nux.h"
#include "Nux/View.h"
#include "Nux/ScrollView.h"
#include "Nux/BaseWindow.h"
#include "Nux/VScrollBar.h"
#include "NuxImage/CairoGraphics.h"

typedef enum
{
  STATE_OFF = 0,
  STATE_OVER,
  STATE_DOWN,
  STATE_LAST
} State;

class PlacesVScrollBar : public nux::VScrollBar
{
  public:
    PlacesVScrollBar (NUX_FILE_LINE_PROTO);
    ~PlacesVScrollBar ();

    void RecvMouseEnter (int           x,
                         int           y,
                         unsigned long button_flags,
                         unsigned long key_flags);

    void RecvMouseLeave (int           x,
                         int           y,
                         unsigned long button_flags,
                         unsigned long key_flags);

    void RecvMouseDown (int           x,
                        int           y,
                        unsigned long button_flags,
                        unsigned long key_flags);

    void RecvMouseUp (int           x,
                      int           y,
                      unsigned long button_flags,
                      unsigned long key_flags);

    void RecvMouseDrag (int           x,
                        int           y,
                        int           dx,
                        int           dy,
                        unsigned long button_flags,
                        unsigned long key_flags);

  protected:
    void PreLayoutManagement ();

    void Draw (nux::GraphicsEngine& gfxContext,
               bool                 forceDraw);

  private:
    void UpdateTexture ();

  private:
    bool                _drag;
    bool                _entered;
    State               _state;
    nux::BaseTexture*   _slider[STATE_LAST];
    nux::BaseTexture*   _track;
};

#endif // PLACES_VSCROLLBAR_H
