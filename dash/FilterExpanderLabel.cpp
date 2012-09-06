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
#include "FilterExpanderLabel.h"
#include "unity-shared/LineSeparator.h"

namespace
{

const float EXPAND_DEFAULT_ICON_OPACITY = 1.0f;

// expander_layout_
const int EXPANDER_LAYOUT_SPACE_BETWEEN_CHILDREN = 8;

// font
const char* const FONT_EXPANDER_LABEL = "Ubuntu Bold 13"; // 17px = 13

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
    bool mouse_inside = TestMousePointerInclusionFilterMouseWheel(mouse_position, event_type);

    if (mouse_inside == false)
      return nullptr;

    return this;
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
  , draw_separator(false)
  , layout_(nullptr)
  , top_bar_layout_(nullptr)
  , expander_view_(nullptr)
  , expander_layout_(nullptr)
  , right_hand_contents_(nullptr)
  , cairo_label_(nullptr)
  , raw_label_(label)
  , label_("label")
  , separator_(nullptr)
{
  expanded.changed.connect(sigc::mem_fun(this, &FilterExpanderLabel::DoExpandChange));
  BuildLayout();

  separator_ = new HSeparator;
  separator_->SinkReference();

  dash::Style& style = dash::Style::Instance();
  int space_height = style.GetSpaceBetweenFilterWidgets() - style.GetFilterHighlightPadding();

  space_ = new nux::SpaceLayout(space_height, space_height, space_height, space_height);
  space_->SinkReference();

  draw_separator.changed.connect([&](bool value)
  {
    if (value and !separator_->IsChildOf(layout_))
    {
      layout_->AddLayout(space_, 0);
      layout_->AddView(separator_, 0);
    }
    else if (!value and separator_->IsChildOf(layout_))
    {
      layout_->RemoveChildObject(space_);
      layout_->RemoveChildObject(separator_);
    }
    QueueDraw();
  });
}

FilterExpanderLabel::~FilterExpanderLabel()
{
  if (space_)
    space_->UnReference();

  if (separator_)
    separator_->UnReference();
}

void FilterExpanderLabel::SetLabel(std::string const& label)
{
  raw_label_ = label;

  cairo_label_->SetText(label.c_str());
}

void FilterExpanderLabel::SetRightHandView(nux::View* view)
{
  dash::Style& style = dash::Style::Instance();

  right_hand_contents_ = view;
  right_hand_contents_->SetMinimumHeight(style.GetAllButtonHeight());
  right_hand_contents_->SetMaximumHeight(style.GetAllButtonHeight());
  top_bar_layout_->AddView(right_hand_contents_, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);
}

void FilterExpanderLabel::SetContents(nux::Layout* contents)
{
  // Since the contents is initially unowned, we don't want to Adopt it, just assign.
  contents_ = contents;

  layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);

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
  top_bar_layout_->AddView(expander_view_, 0);

  cairo_label_ = new nux::StaticCairoText(label_.c_str(), NUX_TRACKER_LOCATION);
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
  top_bar_layout_->AddSpace(1, 1);

  layout_->AddLayout(top_bar_layout_, 0, nux::MINOR_POSITION_LEFT);
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
    layout_->AddLayout(contents_.GetPointer(), 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL, 100.0f, nux::LayoutPosition(1));
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

  if (RedirectedAncestor())
  {
    unsigned int alpha = 0, src = 0, dest = 0;
    graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
    // This is necessary when doing redirected rendering.
    // Clean the area below this view before drawing anything.
    graphics_engine.GetRenderStates().SetBlend(false);
    graphics_engine.QRP_Color(GetX(), GetY(), GetWidth(), GetHeight(), nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
    graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  if (ShouldBeHighlighted())
  {
    nux::Geometry geo(top_bar_layout_->GetGeometry());
    geo.x = base.x;
    geo.width = base.width;

    if (!highlight_layer_)
      highlight_layer_.reset(dash::Style::Instance().FocusOverlay(geo.width, geo.height));

    highlight_layer_->SetGeometry(geo);
    highlight_layer_->Renderlayer(graphics_engine);
  }

  graphics_engine.PopClippingRectangle();
}

void FilterExpanderLabel::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  graphics_engine.PushClippingRectangle(GetGeometry());

  if (RedirectedAncestor() && !IsFullRedraw())
  {
    unsigned int alpha = 0, src = 0, dest = 0;
    graphics_engine.GetRenderStates().GetBlend(alpha, src, dest);
    // This is necessary when doing redirected rendering.
    // Clean the area below this view before drawing anything.
    graphics_engine.GetRenderStates().SetBlend(false);
    graphics_engine.QRP_Color(GetX(), GetY(), GetWidth(), GetHeight(), nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
    graphics_engine.GetRenderStates().SetBlend(alpha, src, dest);
  }

  int pushed_paint_layers = 0;
  if (RedirectedAncestor())
  {
    if (ShouldBeHighlighted() && highlight_layer_ && !IsFullRedraw())
      nux::GetPainter().RenderSinglePaintLayer(graphics_engine, highlight_layer_->GetGeometry(), highlight_layer_.get());
  }
  else if (ShouldBeHighlighted() && highlight_layer_ && !IsFullRedraw())
  {
    ++pushed_paint_layers;
    nux::GetPainter().PushLayer(graphics_engine, highlight_layer_->GetGeometry(), highlight_layer_.get());
  }

  GetLayout()->ProcessDraw(graphics_engine, true);
  graphics_engine.PopClippingRectangle();

  if (pushed_paint_layers)
  {
    nux::GetPainter().PopBackground(pushed_paint_layers);
  }
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

void FilterExpanderLabel::AddProperties(GVariantBuilder* builder)
{
  bool content_has_focus = false;
  auto focus_area = nux::GetWindowCompositor().GetKeyFocusArea();

  if (focus_area && contents_)
    content_has_focus = focus_area->IsChildOf(contents_.GetPointer());

  unity::variant::BuilderWrapper wrapper(builder);

  wrapper.add("expander-has-focus", (expander_view_ && expander_view_->HasKeyFocus()))
         .add("expanded", expanded())
         .add(GetAbsoluteGeometry())
         .add("content-has-focus", content_has_focus);
}

} // namespace dash
} // namespace unity
