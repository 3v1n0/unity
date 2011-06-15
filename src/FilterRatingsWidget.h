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



#ifndef FILTERRATINGSWIDGET_H
#define FILTERRATINGSWIDGET_H

#include <Nux/Nux.h>
#include "Nux/Button.h"
#include "FilterWidget.h"

namespace unity {

  class FilterRatings : public nux::Button, public unity::FilterWidget {
  public:
    FilterRatings (NUX_FILE_LINE_PROTO);
    virtual ~FilterRatings();

    void SetFilter (void *);

    nux::Property<int> rating;

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void InitTheme ();

    void RecvMouseDown (int x, int y, unsigned long button_flags, unsigned long key_flags);
    void OnRatingsChanged (int rating);

  };

}

#endif // FILTERRATINGSWIDGET_H
