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

namespace
{
const float kExpandDefaultIconOpacity = 1.0f;
}

namespace unity
{
namespace dash
{

class ExpanderView : public nux::View
{
public:
  ExpanderView(NUX_FILE_LINE_DECL)
   : nux::View(NUX_FILE_LINE_PARAM)
  {
  }

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
  };

  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
    if (GetLayout())
      GetLayout()->ProcessDraw(graphics_engine, force_draw);
  }

  bool AcceptKeyNavFocus()
  {
    return false;
  }
};


NUX_IMPLEMENT_OBJECT_TYPE(FilterExpanderLabel);

FilterExpanderLabel::FilterExpanderLabel(std::string const& label, NUX_FILE_LINE_DECL)
  : FilterWidget(NUX_FILE_LINE_PARAM)
  , expanded(true)
  , layout_(nullptr)
  , top_bar_layout_(nullptr)
  , expander_view_(nullptr)
  , expander_layout_(nullptr)
  , right_hand_contents_(nullptr)
  , cairo_label_(nullptr)
  , raw_label_(label)
  , label_("<span size='larger' weight='bold'>" + label + "</span>")
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
  cairo_label_->SetText(label_.c_str());
}

void FilterExpanderLabel::SetRightHandView(nux::View* view)
{
  view->SetMinimumHeight(33);

  right_hand_contents_ = view;
  top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
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
  layout_->SetLeftAndRightPadding(3, 1);

  top_bar_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  top_bar_layout_->SetLeftAndRightPadding(2, 0);

  expander_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  expander_layout_->SetSpaceBetweenChildren(8);


  expander_view_ = new ExpanderView(NUX_TRACKER_LOCATION);
  expander_view_->SetLayout(expander_layout_);
  top_bar_layout_->AddView(expander_view_, 0);

  cairo_label_ = new nux::StaticText(label_.c_str(), NUX_TRACKER_LOCATION);
  cairo_label_->SetFontName("Ubuntu 10");
  cairo_label_->SetTextColor(nux::Color(1.0f, 1.0f, 1.0f, 1.0f));
  cairo_label_->SetAcceptKeyNavFocusOnMouseDown(false);

  nux::BaseTexture* arrow;
  arrow = dash::Style::Instance().GetGroupUnexpandIcon();
  expand_icon_ = new IconTexture(arrow,
                                 arrow->GetWidth(),
                                 arrow->GetHeight());
  expand_icon_->SetOpacity(kExpandDefaultIconOpacity);
  expand_icon_->SetMinimumSize(arrow->GetWidth(), arrow->GetHeight());
  expand_icon_->SetVisible(true);
  arrow_layout_  = new nux::VLayout();
  arrow_top_space_ = new nux::SpaceLayout(2, 2, 11, 11);
  arrow_bottom_space_ = new nux::SpaceLayout(2, 2, 9, 9);
  arrow_layout_->AddView(arrow_top_space_, 0, nux::MINOR_POSITION_CENTER);
  arrow_layout_->AddView(expand_icon_, 0, nux::MINOR_POSITION_CENTER);
  arrow_layout_->AddView(arrow_bottom_space_, 0, nux::MINOR_POSITION_CENTER);

  expander_layout_->AddView(cairo_label_, 1, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FULL);
  expander_layout_->AddView(arrow_layout_, 0, nux::MINOR_POSITION_CENTER);
  top_bar_layout_->AddSpace(1, 1);

  top_bar_layout_->SetMaximumWidth((Style::Instance().GetTileWidth() - 12) * 2 + 19);

  layout_->AddLayout(top_bar_layout_, 0, nux::MINOR_POSITION_LEFT);
  layout_->SetVerticalInternalMargin(0);

  SetLayout(layout_);

  // Lambda functions
  auto mouse_redraw = [&](int x, int y, unsigned long b, unsigned long k)
  {
    QueueDraw();
  };

  auto mouse_expand = [&](int x, int y, unsigned long b, unsigned long k)
  {
    expanded = !expanded;
  };

  // Signals
  expander_view_->mouse_click.connect(mouse_expand);
  expander_view_->mouse_enter.connect(mouse_redraw);
  expander_view_->mouse_leave.connect(mouse_redraw);
  cairo_label_->mouse_click.connect(mouse_expand);
  cairo_label_->mouse_enter.connect(mouse_redraw);
  cairo_label_->mouse_leave.connect(mouse_redraw);
  expand_icon_->mouse_click.connect(mouse_expand);
  expand_icon_->mouse_enter.connect(mouse_redraw);
  expand_icon_->mouse_leave.connect(mouse_redraw);

  QueueRelayout();
  NeedRedraw();
}

void FilterExpanderLabel::DoExpandChange(bool change)
{
  dash::Style& style = dash::Style::Instance();
  if (expanded)
    expand_icon_->SetTexture(style.GetGroupUnexpandIcon());
  else
    expand_icon_->SetTexture(style.GetGroupExpandIcon());

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

bool FilterExpanderLabel::ShouldBeHighlighted()
{
  return ((expander_view_ && expander_view_->IsMouseInside()) ||
          (cairo_label_ && cairo_label_->IsMouseInside()) ||
          (expand_icon_ && expand_icon_->IsMouseInside()));
}

void FilterExpanderLabel::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  GfxContext.PushClippingRectangle(base);
  nux::GetPainter().PaintBackground(GfxContext, base);

  if (ShouldBeHighlighted())
  {
    nux::Geometry geo(top_bar_layout_->GetGeometry());
    geo.x = base.x;
    geo.height = 34;
    geo.width = base.width - 5;

    if (!highlight_layer_)
      highlight_layer_.reset(dash::Style::Instance().FocusOverlay(geo.width, geo.height));

    highlight_layer_->SetGeometry(geo);
    highlight_layer_->Renderlayer(GfxContext);
  }

  GfxContext.PopClippingRectangle();
}

void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());

  if (ShouldBeHighlighted() && highlight_layer_ && !IsFullRedraw())
  {
    nux::GetPainter().PushLayer(GfxContext, highlight_layer_->GetGeometry(), highlight_layer_.get());
  }

  GetLayout()->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

} // namespace dash
} // namespace unity
