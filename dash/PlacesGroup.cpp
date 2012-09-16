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
#include <glib/gi18n-lib.h>

#include <UnityCore/Variant.h>
#include <UnityCore/GLibWrapper.h>

#include "unity-shared/StaticCairoText.h"
#include "unity-shared/DashStyle.h"
#include "unity-shared/LineSeparator.h"
#include "unity-shared/ubus-server.h"
#include "unity-shared/UBusMessages.h"


namespace
{
nux::logging::Logger logger("unity.dash.placesgroup");
}

#include "ResultView.h"
#include "ResultViewGrid.h"
#include "ResultRendererTile.h"
#include "ResultRendererHorizontalTile.h"
#include "CoverflowResultView.h"
#include "FilterBasicButton.h"


namespace unity
{
namespace
{

const nux::Color kExpandDefaultTextColor(1.0f, 1.0f, 1.0f, 0.5f);
const float kExpandDefaultIconOpacity = 0.5f;

const int kCategoryIconSize = 22;
// Category  highlight
const int kHighlightHeight = 24;
const int kHighlightRightPadding = 10 - 3; // -3 because the scrollbar is not a real overlay scrollbar!
const int kHighlightLeftPadding = 10;

// Font
const char* const NAME_LABEL_FONT = "Ubuntu 13"; // 17px = 13
const char* const EXPANDER_LABEL_FONT = "Ubuntu 10"; // 13px = 10

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
    if (IsFullRedraw() && GetLayout())
    {
      nux::GetPainter().PushPaintLayerStack();
      GetLayout()->ProcessDraw(graphics_engine, force_draw);
      nux::GetPainter().PopPaintLayerStack();
    }
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

}

NUX_IMPLEMENT_OBJECT_TYPE(PlacesGroup);

PlacesGroup::PlacesGroup()
  : AbstractPlacesGroup(),
    _child_view(nullptr),
    _is_expanded(false),
    _n_visible_items_in_unexpand_mode(0),
    _n_total_items(0),
    _category_index(0),
    _coverflow_enabled(false),
    disabled_header_count_(false)
{
  dash::Style& style = dash::Style::Instance();

  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptKeyNavFocusOnMouseEnter(false);

  nux::BaseTexture* arrow = style.GetGroupUnexpandIcon();
  
  _background = style.GetCategoryBackground();
  _background_nofilters = style.GetCategoryBackgroundNoFilters();

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

  // -2 because the icons have an useless border.
  int top_space = style.GetPlacesGroupTopSpace() - 2;
  _group_layout->AddLayout(new nux::SpaceLayout(top_space, top_space, top_space, top_space), 0);

  _header_view = new HeaderView(NUX_TRACKER_LOCATION);
  _group_layout->AddView(_header_view, 0, nux::MINOR_POSITION_TOP, nux::MINOR_SIZE_FULL);

  _header_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _header_layout->SetLeftAndRightPadding(style.GetCategoryHeaderLeftPadding(), 0);
  _header_layout->SetSpaceBetweenChildren(10);
  _header_view->SetLayout(_header_layout);

  _icon = new IconTexture("", kCategoryIconSize);
  _icon->SetMinMaxSize(kCategoryIconSize, kCategoryIconSize);
  _header_layout->AddView(_icon, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_FIX);

  _text_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _text_layout->SetHorizontalInternalMargin(15);
  _header_layout->AddLayout(_text_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _name = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  _name->SetFont(NAME_LABEL_FONT);
  _name->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _name->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_LEFT);
  _text_layout->AddView(_name, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _expand_layout->SetHorizontalInternalMargin(8);
  _text_layout->AddLayout(_expand_layout, 0, nux::MINOR_POSITION_END, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_label_layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  _expand_layout->AddLayout(_expand_label_layout, 0, nux::MINOR_POSITION_CENTER, nux::MINOR_SIZE_MATCHCONTENT);

  _expand_label = new nux::StaticCairoText("", NUX_TRACKER_LOCATION);
  _expand_label->SetFont(EXPANDER_LABEL_FONT);
  _expand_label->SetTextEllipsize(nux::StaticCairoText::NUX_ELLIPSIZE_END);
  _expand_label->SetTextAlignment(nux::StaticCairoText::NUX_ALIGN_LEFT);
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
  
  key_nav_focus_change.connect([&](nux::Area* area, bool has_focus, nux::KeyNavDirection direction)
  {
    if (!has_focus)
      return;

    if(direction == nux::KEY_NAV_UP)
      nux::GetWindowCompositor().SetKeyFocusArea(_child_view, direction);
    else
      nux::GetWindowCompositor().SetKeyFocusArea(GetHeaderFocusableView(), direction);
  });

  _ubus.RegisterInterest(UBUS_REFINE_STATUS, [this] (GVariant *data) 
  {
    gboolean status;
    g_variant_get(data, UBUS_REFINE_STATUS_FORMAT_STRING, &status);

    nux::ROPConfig rop;
    rop.Blend = true;
    rop.SrcBlend = GL_ONE;
    rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

    nux::TexCoordXForm texxform;
    if (status && _using_nofilters_background)
    {
      _background_layer.reset(new nux::TextureLayer(_background->GetDeviceTexture(), 
                              texxform, 
                              nux::color::White,
                              false,
                              rop));
      _using_nofilters_background = false;
    }
    else if (!status && !_using_nofilters_background)
    {
      _background_layer.reset(new nux::TextureLayer(_background_nofilters->GetDeviceTexture(), 
                              texxform, 
                              nux::color::White,
                              false,
                              rop));
      
      _using_nofilters_background = true;
    }
    QueueDraw();
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

void
PlacesGroup::SetRendererName(const char *renderer_name)
{
  _renderer_name = renderer_name;

  if (g_strcmp0(renderer_name, "tile-horizontal") == 0)
    (static_cast<dash::ResultView*>(_child_view))->SetModelRenderer(new dash::ResultRendererHorizontalTile(NUX_TRACKER_LOCATION));
  else if (g_strcmp0(renderer_name, "tile-vertical") == 0)
    (static_cast<dash::ResultView*>(_child_view))->SetModelRenderer(new dash::ResultRendererTile(NUX_TRACKER_LOCATION));
}

void PlacesGroup::SetHeaderCountVisible(bool disable)
{
  disabled_header_count_ = !disable;
  Relayout();
}

nux::StaticCairoText*
PlacesGroup::GetLabel()
{
  return _name;
}

nux::StaticCairoText*
PlacesGroup::GetExpandLabel()
{
  return _expand_label;
}

void
PlacesGroup::SetIcon(std::string const& path_to_emblem)
{
  _icon->SetByIconName(path_to_emblem, kCategoryIconSize);
}

void
PlacesGroup::SetChildView(dash::ResultView* view)
{
  if (_child_view != NULL)
    {
      _group_layout->RemoveChildObject(_child_view);
    }

  debug::Introspectable *i = dynamic_cast<debug::Introspectable*>(view);
  if (i)
    AddChild(i);

  _child_view = view;

  nux::VLayout* layout = new nux::VLayout();
  layout->AddView(_child_view, 0);

  layout->SetLeftAndRightPadding(25, 0);
  _group_layout->AddLayout(new nux::SpaceLayout(2,2,2,2), 0); // top padding
  _group_layout->AddLayout(layout, 1);

  view->results_per_row.changed.connect([&] (int results_per_row) 
  {
    _n_visible_items_in_unexpand_mode = results_per_row;  
    RefreshLabel();
  });

  QueueDraw();
}

nux::View*
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
      LOG_DEBUG(logger) << _n_total_items << " - " << _n_visible_items_in_unexpand_mode;
      result_string = glib::String(g_strdup_printf(g_dngettext(GETTEXT_PACKAGE,
                                                  "See one more result",
                                                  "See %d more results",
                                                  _n_total_items - _n_visible_items_in_unexpand_mode),
                                      _n_total_items - _n_visible_items_in_unexpand_mode)).Str();
    }
  }

  bool visible = !(_n_visible_items_in_unexpand_mode >= _n_total_items && _n_total_items != 0);

  _expand_icon->SetVisible(visible);;
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
    _focus_layer.reset(dash::Style::Instance().FocusOverlay(geo.width - kHighlightLeftPadding - kHighlightRightPadding, kHighlightHeight));
    _cached_geometry = geo;
  }
  return ret;
}

void PlacesGroup::Draw(nux::GraphicsEngine& graphics_engine,
                       bool                 forceDraw)
{
  nux::Geometry const& base = GetGeometry();
  graphics_engine.PushClippingRectangle(base);

  if (RedirectedAncestor())
  {
    // This is necessary when doing redirected rendering. Clean the area below this view.
    unsigned int current_alpha_blend;
    unsigned int current_src_blend_factor;
    unsigned int current_dest_blend_factor;
    graphics_engine.GetRenderStates().GetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);

    graphics_engine.GetRenderStates().SetBlend(false);
    graphics_engine.QRP_Color(GetX(), GetY(), GetWidth(), GetHeight(), nux::Color(0.0f, 0.0f, 0.0f, 0.0f));

    graphics_engine.GetRenderStates().SetBlend(current_alpha_blend, current_src_blend_factor, current_dest_blend_factor);
  }
  
  if (ShouldBeHighlighted())
  {
    nux::Geometry geo(_header_layout->GetGeometry());
    geo.width = base.width - kHighlightRightPadding - kHighlightLeftPadding;
    geo.x += kHighlightLeftPadding;

    _focus_layer->SetGeometry(geo);
    _focus_layer->Renderlayer(graphics_engine);
  }

  nux::Geometry bg_geo = GetGeometry();
  int bg_width = 0;
  if (_using_nofilters_background)
    bg_width = _background_nofilters->GetWidth();
  else
    bg_width = _background->GetWidth();

  bg_geo.x = std::max(bg_geo.width - bg_width,0);
  
  bg_geo.width = std::min(bg_width, bg_geo.GetWidth()) + 1; // to render into a space left over by the scrollview
  bg_geo.height = _background->GetHeight();
  
  _background_layer->SetGeometry(bg_geo);
  _background_layer->Renderlayer(graphics_engine);
  graphics_engine.PopClippingRectangle();
}

void
PlacesGroup::DrawContent(nux::GraphicsEngine& graphics_engine, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();

  graphics_engine.PushClippingRectangle(base);
  nux::Geometry bg_geo = GetGeometry();
  
  int bg_width = 0;
  if (_using_nofilters_background)
    bg_width = _background_nofilters->GetWidth();
  else
    bg_width = _background->GetWidth();
  
  // if the dash is smaller, resize to fit, otherwise move to the right edge
  bg_geo.x = std::max(bg_geo.width - bg_width, 0);
  bg_geo.width = std::min(bg_width, bg_geo.GetWidth()) + 1; // to render into a space left over by the scrollview
  
  bg_geo.height = _background->GetHeight();

  if (!IsFullRedraw())
  {
    nux::GetPainter().PushLayer(graphics_engine, bg_geo, _background_layer.get());
  }
  if (ShouldBeHighlighted() && !IsFullRedraw() && _focus_layer)
  {
    nux::GetPainter().PushLayer(graphics_engine, _focus_layer->GetGeometry(), _focus_layer.get());
  }

  _group_layout->ProcessDraw(graphics_engine, force_draw);

  graphics_engine.PopClippingRectangle();
}

void PlacesGroup::PostDraw(nux::GraphicsEngine& graphics_engine,
                           bool                 forceDraw)
{
}

void
PlacesGroup::SetCategoryIndex(unsigned index)
{
  _category_index = index;
}

unsigned
PlacesGroup::GetCategoryIndex() const
{
  return _category_index;
}

void
PlacesGroup::SetCounts(unsigned n_visible_items_in_unexpand_mode,
                       unsigned n_total_items)
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

  dash::Style& style = dash::Style::Instance();
  if (_is_expanded)
    _expand_icon->SetTexture(style.GetGroupUnexpandIcon());
  else
    _expand_icon->SetTexture(style.GetGroupExpandIcon());

  expanded.emit(this);
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

void PlacesGroup::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper wrapper(builder);

  wrapper.add("header-x", _header_view->GetAbsoluteX());
  wrapper.add("header-y", _header_view->GetAbsoluteY());
  wrapper.add("header-width", _header_view->GetAbsoluteWidth());
  wrapper.add("header-height", _header_view->GetAbsoluteHeight());
  wrapper.add("header-has-keyfocus", HeaderHasKeyFocus());
  wrapper.add("header-is-highlighted", ShouldBeHighlighted());
  wrapper.add("name", _name->GetText());
  wrapper.add("is-visible", IsVisible());
  wrapper.add("is-expanded", GetExpanded());
  wrapper.add("expand-label-is-visible", _expand_label->IsVisible());
  wrapper.add("expand-label-y", _expand_label->GetAbsoluteY());
  wrapper.add("expand-label-baseline", _expand_label->GetBaseline());
  wrapper.add("name-label-y", _name->GetAbsoluteY());
  wrapper.add("name-label-baseline", _name->GetBaseline());
}

} // namespace unity
