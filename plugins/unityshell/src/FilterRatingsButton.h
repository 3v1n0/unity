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



#ifndef FILTERRATINGSBUTTONWIDGET_H
#define FILTERRATINGSBUTTONWIDGET_H

#include <Nux/Nux.h>
#include "Nux/Button.h"
#include "FilterWidget.h"

namespace unity {

  class FilterRatingsButton : public nux::Button, public unity::FilterWidget {
  public:
    FilterRatingsButton (NUX_FILE_LINE_PROTO);
    virtual ~FilterRatingsButton();

    void SetFilter (void *);
    std::string GetFilterType ();

    nux::Property<int> rating; // maximum of 10

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void InitTheme ();

    void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void OnRatingsChanged (int rating);

    nux::AbstractPaintLayer *_full_prelight;
    nux::AbstractPaintLayer *_full_normal;

    nux::AbstractPaintLayer *_empty_prelight;
    nux::AbstractPaintLayer *_empty_normal;

    nux::AbstractPaintLayer *_half_prelight;
    nux::AbstractPaintLayer *_half_normal;

    void *_filter;

  };

}

#endif // FILTERRATINGSBUTTONWIDGET_H
