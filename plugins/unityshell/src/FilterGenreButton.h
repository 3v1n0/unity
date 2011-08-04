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



#ifndef FILTERGENREBUTTON_H
#define FILTERGENREBUTTON_H

#include <Nux/Nux.h>
#include "Nux/ToggleButton.h"
#include "FilterWidget.h"
#include "FilterBasicButton.h"

namespace unity {

  class FilterGenreButton : public FilterBasicButton {
  public:
    FilterGenreButton (const std::string label, NUX_FILE_LINE_PROTO);
    FilterGenreButton (NUX_FILE_LINE_PROTO);
    void SetFilter (void *);
    std::string GetFilterType ();

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void *_filter;

  };

}
#endif // FILTERGENREBUTTON_H
