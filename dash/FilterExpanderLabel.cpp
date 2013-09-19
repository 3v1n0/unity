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

#include "unity-shared/DashStyle.h"
#include "unity-shared/GraphicsUtils.h"
#include "FilterExpanderLabel.h"

namespace
{

const float EXPAND_DEFAULT_ICON_OPACITY = 1.0f;

// expander_layout_
const int EXPANDER_LAYOUT_SPACE_BETWEEN_CHILDREN = 8;

// font
const char* const FONT_EXPANDER_LABEL = "Ubuntu 13"; // 17px = 13

class ExpanderView : public nux::View
{
public:
  ExpanderView(NUX_FILE_LINE_DECL)
   : nux::View(NUX_FILE_LINE_PARAM)
  {
    SetAcceptKeyNavFocusOnMouseDown(false);
    SetAcceptKeyNavFocusOnMouseEnter(true);
  }

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {};

  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
    if (GetLayout())
      GetLayout()->ProcessDraw(graphics_engine, force_draw);
  }

  bool AcceptKeyNavFocus()
  {
    return true;
  }

  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
  {
    if (event_type != nux::EVENT_MOUSE_WHEEL && TestMousePointerInclusion(mouse_position, event_type))
      return this;
    else
      return nullptr;
  }
};

}

namespace unity
{
namespace dash
{

NUX_IMPLEMENT_OBJECT_TYPE(FilterExpanderLabel);

FilterExpanderLabel::FilterExpanderLabel(std::string const& label, NUX_FILE_LINE_DECL)
  : nux::View(NUX_FILE_LINE_PARAM)
  , expanded(true)
  , layout_(nullptr)
  , top_bar_layout_(nullptr)
  , expander_view_(nullptr)
  , expander_layout_(nullptr)
  , right_hand_contents_(nullptr)
  , cairo_label_(nullptr)
  , raw_label_(label)
  , label_("label")
{
  expanded.changed.connect(sigc::mem_fun(this, &FilterExpanderLabel::DoExpandChange));
  BuildLayout();
}

void FilterExpanderLabel::SetLabel(std::string const& label)
{
  raw_label_ = label;

  cairo_label_->SetText(label.c_str());
}

void FilterExpanderLabel::SetRightHandView(nux::View* view)
{
  dash::Style& style = dash::Style::Instance();
  if (right_hand_contents_)
  {
    top_bar_layout_->RemoveChildObject(right_hand_contents_);
    right_hand_contents_ = nullptr;    
  }
  if (view)
  {
    right_hand_contents_ = view;
    right_hand_contents_->SetMinimumHeight(style.GetAllButtonHeight());
    right_hand_contents_->SetMaximumHeight(style.GetAllButtonHeight());
    top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
  }
}

void FilterExpanderLabel::SetContents(nux::Layout* contents)
{
  // Since the contents is initially unowned, we don't want to Adopt it, just assign.
  contents_ = contents;

  layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  QueueDraw();
}

void FilterExpanderLabel::BuildLayout()
{
  dash::Style& style = dash::Style::Instance();

  layout_ = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout_->SetLeftAndRightPadding(style.GetFilterBarLeftPadding(), style.GetFilterBarRightPadding());

  top_bar_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  top_bar_layout_->SetTopAndBottomPadding(style.GetFilterHighlightPadding());

  expander_layout_ = new nux::HLayout(NUX_TRACKER_LOCATION);
  expander_layout_->SetSpaceBetweenChildren(EXPANDER_LAYOUT_SPACE_BETWEEN_CHILDREN);

  expander_view_ = new ExpanderView(NUX_TRACKER_LOCATION);
  expander_view_->SetLayout(expander_layout_);
  top_bar_layout_->AddView(expander_view_, 1);

  cairo_label_ = new StaticCairoText(label_.c_str(), NUX_TRACKER_LOCATION);
  cairo_label_->SetFont(FONT_EXPANDER_LABEL);
  cairo_label_->SetTextColor(nux::color::White);
  cairo_label_->SetAcceptKeyboardEvent(false);

  nux::BaseTexture* arrow;
  arrow = dash::Style::Instance().GetGroupUnexpandIcon();
  expand_icon_ = new IconTexture(arrow,
                                 arrow->GetWidth(),
                                 arrow->GetHeight());
  expand_icon_->SetOpacity(EXPAND_DEFAULT_ICON_OPACITY);
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

  layout_->AddLayout(top_bar_layout_, 0, nux::MINOR_POSITION_START);
  layout_->SetVerticalInternalMargin(0);

  SetLayout(layout_);

  // Lambda functions
  auto mouse_expand = [&](int x, int y, unsigned long b, unsigned long k)
  {
    expanded = !expanded;
  };

  auto key_redraw = [&](nux::Area*, bool, nux::KeyNavDirection)
  {
    QueueDraw();
  };

  auto key_expand = [&](nux::Area*)
  {
    expanded = !expanded;
  };

  // Signals
  expander_view_->mouse_click.connect(mouse_expand);
  expander_view_->key_nav_focus_change.connect(key_redraw);
  expander_view_->key_nav_focus_activate.connect(key_expand);
  cairo_label_->mouse_click.connect(mouse_expand);
  expand_icon_->mouse_click.connect(mouse_expand);

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
    layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL, 100.0f, nux::LayoutPosition(1));
  }
  else if (!change and contents_ and contents_->IsChildOf(layout_))
  {
    layout_->RemoveChildObject(contents_.GetPointer());
  }

  layout_->ComputeContentSize();
  QueueDraw();
}

bool FilterExpanderLabel::ShouldBeHighlighted()
{
  return ((expander_view_ && expander_view_->HasKeyFocus()));
}

void FilterExpanderLabel::Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);

  if (ShouldBeHighlighted())
  {
    nux::Geometry geo(top_bar_layout_->GetGeometry());
    geo.x = base.x;
    geo.width = base.width;

    if (!focus_layer_)
      focus_layer_.reset(dash::Style::Instance().FocusOverlay(geo.width, geo.height));

    focus_layer_->SetGeometry(geo);
    focus_layer_->Renderlayer(graphics_engine);
  }

  graphics_engine.PopClippingRectangle();
}

void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  graphics_engine.PushClippingRectangle(GetGeometry());

  int pushed_paint_layers = 0;
  if (!IsFullRedraw())
  {
    if (RedirectedAncestor())
    {
      if (cairo_label_->IsRedrawNeeded())
        graphics::ClearGeometry(cairo_label_->GetGeometry());
      if (expand_icon_->IsRedrawNeeded())
        graphics::ClearGeometry(expand_icon_->GetGeometry());
      if (right_hand_contents_ && right_hand_contents_->IsRedrawNeeded())
        graphics::ClearGeometry(right_hand_contents_->GetGeometry());

      if (expanded())
        ClearRedirectedRenderChildArea();            
    }

    if (focus_layer_ && ShouldBeHighlighted())
    {
      ++pushed_paint_layers;
      nux::GetPainter().PushLayer(graphics_engine, focus_layer_->GetGeometry(), focus_layer_.get());
    }
  }
  else
  {
    nux::GetPainter().PushPaintLayerStack();
  }

  GetLayout()->ProcessDraw(graphics_engine, force_draw);

  if (IsFullRedraw())
  {
    nux::GetPainter().PopPaintLayerStack();      
  }
  else if (pushed_paint_layers > 0)
  {
    nux::GetPainter().PopBackground(pushed_paint_layers);
  }

  graphics_engine.PopClippingRectangle();
}

//
// Key navigation
//
bool FilterExpanderLabel::AcceptKeyNavFocus()
{
  return false;
}

//
// Introspection
//
std::string FilterExpanderLabel::GetName() const
{
  return "FilterExpanderLabel";
}

void FilterExpanderLabel::AddProperties(debug::IntrospectionData& introspection)
{
  bool content_has_focus = false;
  auto focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

  if (focus_area && contents_)
    content_has_focus = focus_area->IsChildOf(contents_.GetPointer());

  introspection.add("expander-has-focus", (expander_view_ && expander_view_->HasKeyFocus()))
               .add("expanded", expanded())
               .add(GetAbsoluteGeometry())
               .add("content-has-focus", content_has_focus);
}

} // namespace dash
} // namespace unity
