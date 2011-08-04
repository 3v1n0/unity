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

#include "config.h"
#include "Nux/Nux.h"
#include "Nux/StaticText.h"

#include "FilterBasicButton.h"
#include "FilterExpanderLabel.h"

namespace unity {

  FilterExpanderLabel::FilterExpanderLabel (std::string label, NUX_FILE_LINE_DECL)
      : nux::View (NUX_FILE_LINE_PARAM)
      , expanded (false)
      , top_bar_layout_ (NULL)
      , contents_ (NULL)
      , right_hand_contents_ (NULL)
      , expander_graphic_ (NULL)
      , label_(label)
  {
    BuildLayout ();
  }

  FilterExpanderLabel::~FilterExpanderLabel() {

    if (right_hand_contents_)
      right_hand_contents_->UnReference();

    if (contents_)
      contents_->UnReference();
  }

  void FilterExpanderLabel::SetLabel (std::string label)
  {
    label_ = label;

    BuildLayout ();
    g_debug ("adding label");
  }

  void FilterExpanderLabel::SetRightHandView (nux::View *view)
  {
    g_debug ("adding right hand view");
    view->Reference ();

    if (right_hand_contents_)
      right_hand_contents_->UnReference();
    right_hand_contents_ = view;

    BuildLayout ();
  }

  void FilterExpanderLabel::SetContents (nux::Layout *layout)
  {
    g_debug ("adding contents");
    layout->Reference();

    if (contents_)
      contents_->UnReference();
    contents_ = layout;

    BuildLayout ();
  }

  void FilterExpanderLabel::BuildLayout ()
  {
    nux::VLayout *layout = new nux::VLayout(NUX_TRACKER_LOCATION);
    top_bar_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);

    nux::StaticText *cairo_label_ = new nux::StaticText(label_.c_str(), NUX_TRACKER_LOCATION);
    cairo_label_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
    cairo_label_->OnMouseDown.connect(
      [&](int x, int y, unsigned long button_flags, unsigned long key_flag)
      {
        expanded = !expanded;
      });

    top_bar_layout_->AddView (cairo_label_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_MATCHCONTENT);
    top_bar_layout_->AddSpace(1, 1);

    //FIXME - fake any button because nux is being crap
    top_bar_layout_->AddView(new FilterBasicButton("Any", NUX_TRACKER_LOCATION), 0, nux::MINOR_POSITION_RIGHT, nux::MINOR_SIZE_MATCHCONTENT);

    top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_MATCHCONTENT);

    layout->AddLayout (top_bar_layout_, 1);

    if (expanded)
      layout->AddLayout (contents_, 1);


    SetLayout(layout);
  }

  void FilterExpanderLabel::DoExpandChange (bool change)
  {
    BuildLayout();
  }

  long int FilterExpanderLabel::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return PostProcessEvent2 (ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterExpanderLabel::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
  }

  void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry base = GetGeometry ();
    GfxContext.PushClippingRectangle (base);

    if (GetCompositionLayout ())
      GetCompositionLayout ()->ProcessDraw (GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterExpanderLabel::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
