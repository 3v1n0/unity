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



#ifndef FILTEREXPANDERLABEL_H
#define FILTEREXPANDERLABEL_H

#include <Nux/Nux.h>
#include <Nux/GridHLayout.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include "Nux/StaticText.h"

namespace unity {

  class FilterExpanderLabel : public nux::View {
  public:
    FilterExpanderLabel (std::string label, NUX_FILE_LINE_PROTO);
    virtual ~FilterExpanderLabel();

    void SetRightHandView (nux::View *view);
    void SetLabel (std::string label);
    void SetContents (nux::Layout *layout);

    nux::Property<bool> expanded;

  protected:
    virtual long int ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo);
    virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
    virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  private:
    void BuildLayout ();
    void DoExpandChange (bool change);

    nux::Layout* layout_;
    nux::Layout* top_bar_layout_;
    nux::Layout* contents_;
    nux::View* right_hand_contents_;
    nux::View* expander_graphic_;
    nux::StaticText* cairo_label_;
    std::string label_;
  };

}

#endif // FILTEREXPANDERLABEL_H
