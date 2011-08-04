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



#ifndef FILTERGENREWIDGET_H
#define FILTERGENREWIDGET_H

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include "FilterWidget.h"
#include "FilterExpanderLabel.h"

namespace unity {
  class FilterBasicButton;

  class FilterGenre : public FilterExpanderLabel, public unity::FilterWidget {
  public:
    FilterGenre (NUX_FILE_LINE_PROTO);
    virtual ~FilterGenre();

    void SetFilter (void *);
    std::string GetFilterType ();

    nux::Property<bool> all_selected;

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

    void InitTheme ();
    void *_filter;

  private:
    void OnAllActivated(nux::View* view);
    void OnGenreActivated(nux::View* view);
    void BuildGenreLayout();
    nux::GridHLayout* genre_layout_;
    FilterBasicButton* all_button_;
  };

}

#endif // FILTERGENRESWIDGET_H
