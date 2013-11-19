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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <NuxCore/Math/MathInc.h>

#include "PlacesGroup.h"
#include <glib.h>
#include "config.h"
#include <glib/gi18n-lib.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/StaticCairoText.h"
#include "unity-shared/UBusWrapper.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/GraphicsUtils.h"

#include "ResultView.h"
#include "ResultViewGrid.h"
#include "ResultRendererTile.h"
#include "ResultRendererHorizontalTile.h"
#include "CoverflowResultView.h"
#include "FilterBasicButton.h"


DECLARE_LOGGER(logger, "unity.dash.placesgroup");
namespace unity
{
namespace dash
{
namespace
{

const nux::Color kExpandDefaultTextColor(1.0f, 1.0f, 1.0f, 0.5f);
const float kExpandDefaultIconOpacity = 0.5f;

// Category  highlight
const int kHighlightHeight = 24;
const int kHighlightRightPadding = 10 - 3; // -3 because the scrollbar is not a real overlay scrollbar!
const int kHighlightLeftPadding = 10;

// Font
const char* const NAME_LABEL_FONT = "Ubuntu 13"; // 17px = 13
const char* const EXPANDER_LABEL_FONT = "Ubuntu 10"; // 13px = 10

}

class HeaderView : public nux::View
{
public:
  HeaderView(NUX_FILE_LINE_DECL)
   : nux::View(NUX_FILE_LINE_PARAM)
  {
    SetAcceptKeyNavFocusOnMouseDown(false);
    SetAcceptKeyNavFocusOnMouseEnter(true);
  }

protected:
  void Draw(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
  };

  void DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
  {
    graphics_engine.PushClippingRectangle(GetGeometry());
    nux::GetPainter().PushPaintLayerStack();

    if (GetLayout())
    {
      GetLayout()->ProcessDraw(graphics_engine, force_draw);
    }

    nux::GetPainter().PopPaintLayerStack();
    graphics_engine.PopClippingRectangle();
  }

  bool AcceptKeyNavFocus()
  {
    return true;
  }

  nux::Area* FindAreaUnderMouse(const nux::Point& mouse_position, nux::NuxEventType event_type)
  {
    if (event_type != nux::EVENT_MOUSE_WHEEL &&
        TestMousePointerInclusion(mouse_position, event_type))
      return this;
    else
      return nullptr;
  }
};

NUX_IMPLEMENT_OBJECT_TYPE(PlacesGroup);

PlacesGroup::PlacesGroup(dash::StyleInterface& style)
  : nux::View(NUX_TRACKER_LOCATION),
    _style(style),
    _child_layout(nullptr),
    _child_view(nullptr),
    _using_filters_background(false),
    _is_expanded(false),
    _is_expanded_pushed(false),
    _n_visible_items_in_unexpand_mode(0),
    _n_total_items(0),
    _coverflow_enabled(false),
    disabled_header_count_(false)
{
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);

  nux::BaseTexture* arrow = _style.GetGroupExpandIcon();
  
  _background = _style.GetCategoryBackground();
  _background_nofilters = _style.GetCategoryBackgroundNoFilters();

  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::TexCoordXForm texxform;
  _background_layer.reset(new nux::TextureLayer(_background_nofilters->GetDeviceTexture(), 
                          texxform, 
                          nux::color::White,
                          false,
                          rop));

  _group_layout = new nux::VLayout("", NUX_TRACKER_LOCATION);

  int top_space = style.GetPlacesGroupTopSpace();
  _group_layout->AddLayout(new nux::SpaceLayout(top_space, top_space, top_space, top_space), 0);

  _header_view = new HeaderView(NUX_TRACKER_LOCATION);
  _group_layout->AddView(_header_view, 0, nux::MINOR_POSITION_START, nux::MINOR_SIZE_FULL);

  _header_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _header_layout->SetLeftAndRightPadding(_style.GetCategoryHeaderLeftPadding(), 0);
  _header_layout->SetSpaceBetweenChildren(10);
  _header_view->SetLayout(_header_layout);

  _icon = new IconTexture("", _style.GetCategoryIconSize());
  _icon->SetMinMaxSize(_style.GetCategoryIconSize(), _style.GetCategoryIconSize());
  _header_layout->AddView(_icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _text_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _text_layout->SetHorizontalInternalMargin(15);
  _header_layout->AddLayout(_text_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _name = new StaticCairoText("", NUX_TRACKER_LOCATION);
  _name->SetFont(NAME_LABEL_FONT);
  _name->SetTextEllipsize(StaticCairoText::NUX_ELLIPSIZE_END);
  _name->SetTextAlignment(StaticCairoText::NUX_ALIGN_LEFT);
  _text_layout->AddView(_name, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _expand_layout->SetHorizontalInternalMargin(8);
  _text_layout->AddLayout(_expand_layout, 0, nux::MINOR_POSITION_END, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_label_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _expand_layout->AddLayout(_expand_label_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_label = new StaticCairoText("", NUX_TRACKER_LOCATION);
  _expand_label->SetFont(EXPANDER_LABEL_FONT);
  _expand_label->SetTextEllipsize(StaticCairoText::NUX_ELLIPSIZE_END);
  _expand_label->SetTextAlignment(StaticCairoText::NUX_ALIGN_LEFT);
  _expand_label->SetTextColor(kExpandDefaultTextColor);
  _expand_label_layout->AddView(_expand_label, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _expand_icon = new IconTexture(arrow, arrow->GetWidth(), arrow->GetHeight());
  _expand_icon->SetOpacity(kExpandDefaultIconOpacity);
  _expand_icon->SetMinimumSize(arrow->GetWidth(), arrow->GetHeight());
  _expand_icon->SetVisible(false);
  _expand_layout->AddView(_expand_icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  SetLayout(_group_layout);

  // don't need to disconnect these signals as they are disconnected when this object destroys the contents
  _header_view->mouse_click.connect(sigc::mem_fun(this, &PlacesGroup::RecvMouseClick));
  _header_view->key_nav_focus_change.connect(sigc::mem_fun(this, &PlacesGroup::OnLabelFocusChanged));
  _header_view->key_nav_focus_activate.connect(sigc::mem_fun(this, &PlacesGroup::OnLabelActivated));
  _icon->mouse_click.connect(sigc::mem_fun(this, &PlacesGroup::RecvMouseClick));
  _name->mouse_click.connect(sigc::mem_fun(this, &PlacesGroup::RecvMouseClick));
  _expand_label->mouse_click.connect(sigc::mem_fun(this, &PlacesGroup::RecvMouseClick));
  _expand_icon->mouse_click.connect(sigc::mem_fun(this, &PlacesGroup::RecvMouseClick));

  key_nav_focus_change.connect([this](nux::Area* area, bool has_focus, nux::KeyNavDirection direction)
  {
    if (!has_focus)
      return;

    if(direction == nux::KEY_NAV_UP)
      nux::GetWindowCompositor().SetKeyFocusArea(_child_view, direction);
    else
      nux::GetWindowCompositor().SetKeyFocusArea(GetHeaderFocusableView(), direction);
  });
}

void
PlacesGroup::OnLabelActivated(nux::Area* label)
{
  SetExpanded(!_is_expanded);
}

void
PlacesGroup::OnLabelFocusChanged(nux::Area* label, bool has_focus, nux::KeyNavDirection direction)
{
  if (HeaderHasKeyFocus())
  {
    _ubus.SendMessage(UBUS_RESULT_VIEW_KEYNAV_CHANGED,
                      g_variant_new("(iiii)", 0, 0, 0, 0));
  }

  QueueDraw();
}

void
PlacesGroup::SetName(std::string const& name)
{
  if (_cached_name != name)
  {
    _cached_name = name;
    _name->SetText(glib::String(g_markup_escape_text(name.c_str(), -1)).Str());
  }
}

void PlacesGroup::SetHeaderCountVisible(bool disable)
{
  disabled_header_count_ = !disable;
  Relayout();
}

StaticCairoText*
PlacesGroup::GetLabel()
{
  return _name;
}

StaticCairoText*
PlacesGroup::GetExpandLabel()
{
  return _expand_label;
}

void
PlacesGroup::SetIcon(std::string const& path_to_emblem)
{
  _icon->SetByIconName(path_to_emblem, _style.GetCategoryIconSize());
}

void
PlacesGroup::SetChildView(dash::ResultView* view)
{
  if (_child_view)
  {
    RemoveChild(_child_view);
  }
  if (_child_layout != NULL)
  {
    _group_layout->RemoveChildObject(_child_layout);
  }
  AddChild(view);

  _child_view = view;

  _child_layout = new nux::VLayout();
  _child_layout->AddView(_child_view, 0);

  _child_layout->SetTopAndBottomPadding(_style.GetPlacesGroupResultTopPadding(),0);
  _child_layout->SetLeftAndRightPadding(_style.GetPlacesGroupResultLeftPadding(), 0);
  _group_layout->AddLayout(_child_layout, 1);

  view->results_per_row.changed.connect([this] (int results_per_row)
  {
    _n_visible_items_in_unexpand_mode = results_per_row;
    RefreshLabel();
  });

  QueueDraw();
}

dash::ResultView*
PlacesGroup::GetChildView()
{
  return _child_view;
}

void PlacesGroup::SetChildLayout(nux::Layout* layout)
{
  _group_layout->AddLayout(layout, 1);
  QueueDraw();
}

void
PlacesGroup::RefreshLabel()
{
  if (disabled_header_count_)
  {
    _expand_icon->SetVisible(false);
    _expand_label->SetVisible(false);
    return;
  }

  std::string result_string;

  if (_n_visible_items_in_unexpand_mode < _n_total_items)
  {
    if (_is_expanded)
    {
      result_string = _("See fewer results");
    }
    else
    {
      LOG_TRACE(logger) << _n_total_items << " - " << _n_visible_items_in_unexpand_mode;
      result_string = glib::String(g_strdup_printf(g_dngettext(GETTEXT_PACKAGE,
                                                  "See one more result",
                                                  "See %d more results",
                                                  _n_total_items - _n_visible_items_in_unexpand_mode),
                                      _n_total_items - _n_visible_items_in_unexpand_mode)).Str();
    }
  }

  bool visible = !(_n_visible_items_in_unexpand_mode >= _n_total_items && _n_total_items != 0);

  _expand_icon->SetVisible(visible);
  SetName(_cached_name);

  _expand_label->SetText(result_string);
  _expand_label->SetVisible(visible);

  // See bug #748101 ("Dash - "See more..." line should be base-aligned with section header")
  // We're making two assumptions here:
  // [a] The font size _name is bigger than the font size of _expand_label
  // [b] The bottom sides have the same y coordinate
  int bottom_padding = _name->GetBaseHeight() - _name->GetBaseline() -
                       (_expand_label->GetBaseHeight() - _expand_label->GetBaseline());

  _expand_label_layout->SetTopAndBottomPadding(0, bottom_padding);

  QueueDraw();
}

void
PlacesGroup::Refresh()
{
  RefreshLabel();
  ComputeContentSize();
  QueueDraw();
}


void
PlacesGroup::Relayout()
{
  if (_relayout_idle)
    return;

  _relayout_idle.reset(new glib::Idle(glib::Source::Priority::HIGH));
  _relayout_idle->Run(sigc::mem_fun(this, &PlacesGroup::OnIdleRelayout));
}

bool
PlacesGroup::OnIdleRelayout()
{
  if (GetChildView())
  {
    Refresh();
    QueueDraw();
    _group_layout->QueueDraw();
    GetChildView()->QueueDraw();
    ComputeContentSize();
    _relayout_idle.reset();
  }

  return false;
}

long PlacesGroup::ComputeContentSize()
{
  long ret = nux::View::ComputeContentSize();

  nux::Geometry const& geo = GetGeometry();

  // only the width matters
  if (_cached_geometry.GetWidth() != geo.GetWidth())
  {
    _focus_layer.reset(_style.FocusOverlay(geo.width - kHighlightLeftPadding - kHighlightRightPadding, kHighlightHeight));
    _cached_geometry = geo;
  }
  return ret;
}

void PlacesGroup::Draw(nux::GraphicsEngine& graphics_engine,
                       bool                 forceDraw)
{
  nux::Geometry const& base(GetGeometry());
  graphics_engine.PushClippingRectangle(base);

  if (RedirectedAncestor())
    graphics::ClearGeometry(GetGeometry());

  if (ShouldBeHighlighted() && _focus_layer)
  {
    nux::Geometry geo(_header_layout->GetGeometry());
    geo.width = base.width - kHighlightRightPadding - kHighlightLeftPadding;
    geo.x += kHighlightLeftPadding;

    _focus_layer->SetGeometry(geo);
    _focus_layer->Renderlayer(graphics_engine);
  }

  if (_background_layer)
  {
    nux::Geometry bg_geo = base;
    int bg_width = _background_layer->GetDeviceTexture()->GetWidth();
    bg_geo.x = std::max(bg_geo.width - bg_width,0);
    
    bg_geo.width = std::min(bg_width, bg_geo.GetWidth()) + 1; // to render into a space left over by the scrollview
    bg_geo.height = _background->GetHeight();

    _background_layer->SetGeometry(bg_geo);
    _background_layer->Renderlayer(graphics_engine);
  }

  graphics_engine.PopClippingRectangle();
}

void
PlacesGroup::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  graphics_engine.PushClippingRectangle(base);

  int pushed_paint_layers = 0;
  if (!IsFullRedraw())
  {
    if (RedirectedAncestor())
    {
      // Bit tedious. Need to clear the area of the redirected window taken by views
      if (_icon->IsRedrawNeeded())
        graphics::ClearGeometry(_icon->GetGeometry());
      if (_name->IsRedrawNeeded())
        graphics::ClearGeometry(_name->GetGeometry());
      if (_expand_label->IsRedrawNeeded())
        graphics::ClearGeometry(_expand_label->GetGeometry());
      if (_expand_icon->IsRedrawNeeded())
        graphics::ClearGeometry(_expand_icon->GetGeometry());
      if (_child_view && _child_view->IsRedrawNeeded())
        graphics::ClearGeometry(_child_view->GetGeometry());
    }

    if (ShouldBeHighlighted() && _focus_layer)
    {
      ++pushed_paint_layers;
      nux::GetPainter().PushLayer(graphics_engine, _focus_layer->GetGeometry(), _focus_layer.get());
    }
    if (_background_layer)
    {
      ++pushed_paint_layers;
      nux::GetPainter().PushLayer(graphics_engine, _background_layer->GetGeometry(), _background_layer.get());
    }
  }
  else
  {
    nux::GetPainter().PushPaintLayerStack();    
  }

  _group_layout->ProcessDraw(graphics_engine, force_draw);

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

void
PlacesGroup::SetCounts(unsigned n_total_items)
{
  _n_total_items = n_total_items;

  Relayout();
}

bool
PlacesGroup::GetExpanded() const
{
  return _is_expanded;
}

void
PlacesGroup::SetExpanded(bool is_expanded)
{
  if (_is_expanded == is_expanded)
    return;

  if (is_expanded && _n_total_items <= _n_visible_items_in_unexpand_mode)
    return;

  _is_expanded = is_expanded;

  Refresh();

  if (_is_expanded)
    _expand_icon->SetTexture(_style.GetGroupUnexpandIcon());
  else
    _expand_icon->SetTexture(_style.GetGroupExpandIcon());

  expanded.emit(this);
}

void
PlacesGroup::PushExpanded()
{
  _is_expanded_pushed = GetExpanded();
}

void
PlacesGroup::PopExpanded()
{
  SetExpanded(_is_expanded_pushed);
}

void
PlacesGroup::RecvMouseClick(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetExpanded(!_is_expanded);
}

void
PlacesGroup::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw();
}

void
PlacesGroup::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  QueueDraw();
}

int
PlacesGroup::GetHeaderHeight() const
{
  return _header_layout->GetGeometry().height;
}

bool PlacesGroup::HeaderHasKeyFocus() const
{
  return (_header_view && _header_view->HasKeyFocus());
}

bool PlacesGroup::HeaderIsFocusable() const
{
  return (_header_view != nullptr);
}

nux::View* PlacesGroup::GetHeaderFocusableView() const
{
  return _header_view;
}

bool PlacesGroup::ShouldBeHighlighted() const
{
  return HeaderHasKeyFocus();
}

void PlacesGroup::SetResultsPreviewAnimationValue(float preview_animation)
{
  if (_child_view)
    _child_view->desaturation_progress = preview_animation;
}

void PlacesGroup::SetFiltersExpanded(bool filters_expanded)
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::TexCoordXForm texxform;
  if (filters_expanded && !_using_filters_background)
  {
    _background_layer.reset(new nux::TextureLayer(_background->GetDeviceTexture(),
                            texxform, 
                            nux::color::White,
                            false,
                            rop));
  }
  else if (!filters_expanded && _using_filters_background)
  {
    _background_layer.reset(new nux::TextureLayer(_background_nofilters->GetDeviceTexture(),
                            texxform,
                            nux::color::White,
                            false,
                            rop));
  }

  _using_filters_background = filters_expanded;
  QueueDraw();
}

//
// Key navigation
//
bool
PlacesGroup::AcceptKeyNavFocus()
{
  return true;
}

//
// Introspection
//
std::string PlacesGroup::GetName() const
{
  return "PlacesGroup";
}

void PlacesGroup::AddProperties(debug::IntrospectionData& wrapper)
{
  wrapper.add("header-x", _header_view->GetAbsoluteX());
  wrapper.add("header-y", _header_view->GetAbsoluteY());
  wrapper.add("header-width", _header_view->GetAbsoluteWidth());
  wrapper.add("header-height", _header_view->GetAbsoluteHeight());
  wrapper.add("header-geo", _header_view->GetAbsoluteGeometry());
  wrapper.add("header-has-keyfocus", HeaderHasKeyFocus());
  wrapper.add("header-is-highlighted", ShouldBeHighlighted());
  wrapper.add("name", _name->GetText());
  wrapper.add("is-visible", IsVisible());
  wrapper.add("is-expanded", GetExpanded());
  wrapper.add("expand-label-is-visible", _expand_label->IsVisible());
  wrapper.add("expand-label-y", _expand_label->GetAbsoluteY());
  wrapper.add("expand-label-geo", _expand_label->GetAbsoluteGeometry());
  wrapper.add("expand-label-baseline", _expand_label->GetBaseline());
  wrapper.add("name-label-y", _name->GetAbsoluteY());
  wrapper.add("name-label-baseline", _name->GetBaseline());
  wrapper.add("name-label-geo", _name->GetAbsoluteGeometry());
}

glib::Variant PlacesGroup::GetCurrentFocus() const
{
  if (_header_view && _header_view->HasKeyFocus())
  {
    return glib::Variant("HeaderView");
  }
  else if (_child_view && _child_view->HasKeyFocus())
  {
    return g_variant_new("(si)", "ResultView", _child_view->GetSelectedIndex());
  }
  return nullptr;
}

void PlacesGroup::SetCurrentFocus(glib::Variant const& variant)
{
  if (g_variant_is_of_type(variant, G_VARIANT_TYPE_STRING))
  {
    std::string str = glib::gchar_to_string(g_variant_get_string(variant, NULL));
    if (str == "HeaderView" && _header_view)
      nux::GetWindowCompositor().SetKeyFocusArea(_header_view);
  }
  else if (g_variant_is_of_type(variant, G_VARIANT_TYPE("(si)")))
  {
    glib::String str;

    gint32 index;
    g_variant_get(variant, "(si)", &str, &index);
    if (str.Str() == "ResultView" && _child_view)
    {
      _child_view->SetSelectedIndex(index);
      nux::GetWindowCompositor().SetKeyFocusArea(_child_view);
    }
  }
}

} // namespace dash
} // namespace unity
