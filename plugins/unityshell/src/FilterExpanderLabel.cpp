// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#include "DashStyle.h"
#include "FilterBasicButton.h"
#include "FilterExpanderLabel.h"

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterExpanderLabel);

FilterExpanderLabel::FilterExpanderLabel(std::string const& label, NUX_FILE_LINE_DECL)
  : FilterWidget(NUX_FILE_LINE_PARAM)
  , expanded(true)
  , layout_(nullptr)
  , top_bar_layout_(nullptr)
  , right_hand_contents_(nullptr)
  , expander_graphic_(nullptr)
  , cairo_label_(nullptr)
  , raw_label_(label)
  , label_("<span size='larger' weight='bold'>" + label + "</span>" + "  ▾")
{
  expanded.changed.connect(sigc::mem_fun(this, &FilterExpanderLabel::DoExpandChange));
  BuildLayout();
}

FilterExpanderLabel::~FilterExpanderLabel()
{
}

void FilterExpanderLabel::SetLabel(std::string const& label)
{
  raw_label_ = label;

  label_ = "<span size='larger' weight='bold'>";
  label_ += raw_label_;
  label_ += "</span>";
  label_ += expanded ? "  ▾" : "  ▸";
  cairo_label_->SetText(label_.c_str());
}

void FilterExpanderLabel::SetRightHandView(nux::View* view)
{
  view->SetMaximumHeight(30);

  right_hand_contents_ = view;
  top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
}

void FilterExpanderLabel::SetContents(nux::Layout* contents)
{
  contents_.Adopt(contents);

  layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  top_bar_layout_->SetTopAndBottomPadding(0);

  QueueDraw();
}

void FilterExpanderLabel::BuildLayout()
{
  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  top_bar_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);

  cairo_label_ = new nux::StaticText(label_.c_str(), NUX_TRACKER_LOCATION);
  cairo_label_->SetFontName("Ubuntu 10");
  cairo_label_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  cairo_label_->mouse_down.connect(
    [&](int x, int y, unsigned long button_flags, unsigned long key_flag)
    {
      expanded = !expanded;
    });

  top_bar_layout_->AddView(cairo_label_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  top_bar_layout_->AddSpace(1, 1);

  top_bar_layout_->SetMaximumWidth((Style::Instance().GetTileWidth() - 12) * 2 + 10);

  layout_->AddLayout(top_bar_layout_, 0, nux::MINOR_POSITION_LEFT);
  layout_->SetVerticalInternalMargin(0);

  SetLayout(layout_);

  QueueRelayout();
  NeedRedraw();
}

void FilterExpanderLabel::DoExpandChange(bool change)
{
  label_ = "<span size='larger' weight='bold'>";
  label_ += raw_label_;
  label_ += "</span>";
  label_ += expanded ? "  ▾" : "  ▸";

  if (cairo_label_)
    cairo_label_->SetText(label_);

  if (change and contents_ and !contents_->IsChildOf(layout_))
  {
    layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
    top_bar_layout_->SetTopAndBottomPadding(0);
  }
  else if (!change and contents_ and contents_->IsChildOf(layout_))
  {
    layout_->RemoveChildObject(contents_.GetPointer());
    top_bar_layout_->SetTopAndBottomPadding(0, 10);
  }

  layout_->ComputeContentSize();
  QueueDraw();
}

void FilterExpanderLabel::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  GetLayout()->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

} // namespace dash
} // namespace unity
