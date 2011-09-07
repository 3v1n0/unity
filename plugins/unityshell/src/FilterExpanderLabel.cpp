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

#include "FilterBasicButton.h"
#include "FilterExpanderLabel.h"

namespace unity {

  FilterExpanderLabel::FilterExpanderLabel (std::string label, NUX_FILE_LINE_DECL)
      : nux::View (NUX_FILE_LINE_PARAM)
      , expanded (true)
      , top_bar_layout_ (NULL)
      , contents_ (NULL)
      , right_hand_contents_ (NULL)
      , expander_graphic_ (NULL)
      , label_("<span font_size='larger'>" + label + "</span>")
  {
    expanded.changed.connect (sigc::mem_fun(this, &FilterExpanderLabel::DoExpandChange));
    BuildLayout ();
  }

  FilterExpanderLabel::~FilterExpanderLabel() {

  }

  void FilterExpanderLabel::SetLabel (std::string label)
  {
    label_ = "<span font_size='larger'>" + label + "</span>";
    cairo_label_->SetText(label_.c_str());
  }

  void FilterExpanderLabel::SetRightHandView (nux::View *view)
  {
    right_hand_contents_ = view;
    top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  }

  void FilterExpanderLabel::SetContents (nux::Layout *contents)
  {
    contents_ = contents;
    contents_->SetVisible(expanded);
    layout_->AddLayout (contents_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

    QueueDraw();
  }

  void FilterExpanderLabel::BuildLayout ()
  {
    layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
    top_bar_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);

    cairo_label_ = new nux::StaticText(label_.c_str(), NUX_TRACKER_LOCATION);
    cairo_label_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
    cairo_label_->mouse_down.connect(
      [&](int x, int y, unsigned long button_flags, unsigned long key_flag)
      {
        expanded = !expanded;
        if (contents_)
          contents_->SetVisible(expanded);
        QueueDraw();
      });

    top_bar_layout_->AddView (cairo_label_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    top_bar_layout_->AddSpace(1, 1);

    layout_->AddLayout (top_bar_layout_, 0);
    layout_->SetVerticalInternalMargin(12);

    SetLayout(layout_);

    QueueRelayout();
    NeedRedraw();
  }

  void FilterExpanderLabel::DoExpandChange (bool change)
  {
    if (contents_)
      contents_->SetVisible(change);

    QueueRelayout();
  }

  long int FilterExpanderLabel::ProcessEvent(nux::IEvent& ievent, long int TraverseInfo, long int ProcessEventInfo) {
    return GetLayout()->ProcessEvent(ievent, TraverseInfo, ProcessEventInfo);
  }

  void FilterExpanderLabel::Draw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::Geometry geo = GetGeometry();

    GfxContext.PushClippingRectangle(geo);
    nux::GetPainter().PaintBackground(GfxContext, geo);
    GfxContext.PopClippingRectangle();
  }

  void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw) {
    GfxContext.PushClippingRectangle(GetGeometry());

    GetLayout()->ProcessDraw(GfxContext, force_draw);

    GfxContext.PopClippingRectangle();
  }

  void FilterExpanderLabel::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw) {
    nux::View::PostDraw(GfxContext, force_draw);
  }

};
