/*
 * Copyright (C) 2010 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Neil Jagdish Patel <neil.patel@canonical.com>
 */

#include "DashView.h"

#include <math.h>

#include <gio/gdesktopappinfo.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>
#include <UnityCore/RadioOptionFilter.h>
#include <boost/algorithm/string.hpp>

#include "DashStyle.h"
#include "DashSettings.h"
#include "UBusMessages.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.view");
}

NUX_IMPLEMENT_OBJECT_TYPE(DashView);

DashView::DashView()
  : nux::View(NUX_TRACKER_LOCATION)
  , active_lens_view_(0)
  , last_activated_uri_("")
  , visible_(false)

{
  SetupBackground();
  SetupViews();
  SetupUBusConnections();

  Settings::Instance().changed.connect(sigc::mem_fun(this, &DashView::Relayout));
  lenses_.lens_added.connect(sigc::mem_fun(this, &DashView::OnLensAdded));
  mouse_down.connect(sigc::mem_fun(this, &DashView::OnMouseButtonDown));

  Relayout();
  lens_bar_->Activate("home.lens");

  bg_effect_helper_.owner = this;
  bg_effect_helper_.enabled = false;
}

DashView::~DashView()
{
  delete bg_layer_;
  delete bg_darken_layer_;
}

void DashView::AboutToShow()
{
  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
  visible_ = true;
  bg_effect_helper_.enabled = true;
  search_bar_->text_entry()->SelectAll();
}

void DashView::AboutToHide()
{
  visible_ = false;
  bg_effect_helper_.enabled = false;
}

void DashView::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.9), true, rop);

  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_COLOR;
  bg_darken_layer_ = new nux::ColorLayer(nux::Color(0.7f, 0.7f, 0.7f, 1.0f), false, rop);

  bg_shine_texture_ = dash::Style::Instance().GetDashShine()->GetDeviceTexture();

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
}

void DashView::SetupViews()
{
  layout_ = new nux::VLayout();
  SetLayout(layout_);

  content_layout_ = new nux::VLayout();
  content_layout_->SetHorizontalExternalMargin(1);
  content_layout_->SetVerticalExternalMargin(1);

  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
  search_bar_ = new SearchBar();
  search_bar_->activated.connect(sigc::mem_fun(this, &DashView::OnEntryActivated));
  search_bar_->search_changed.connect(sigc::mem_fun(this, &DashView::OnSearchChanged));
  search_bar_->live_search_reached.connect(sigc::mem_fun(this, &DashView::OnLiveSearchReached));
  search_bar_->showing_filters.changed.connect([&] (bool showing) { if (active_lens_view_) active_lens_view_->filters_expanded = showing; QueueDraw(); });
  content_layout_->AddView(search_bar_, 0, nux::MINOR_POSITION_LEFT);

  lenses_layout_ = new nux::VLayout();
  content_layout_->AddView(lenses_layout_, 1, nux::MINOR_POSITION_LEFT);

  home_view_ = new HomeView();
  active_lens_view_ = home_view_;
  lens_views_["home.lens"] = home_view_;
  lenses_layout_->AddView(home_view_);

  lens_bar_ = new LensBar();
  lens_bar_->lens_activated.connect(sigc::mem_fun(this, &DashView::OnLensBarActivated));
  content_layout_->AddView(lens_bar_, 0, nux::MINOR_POSITION_CENTER);
}

void DashView::SetupUBusConnections()
{
  ubus_manager_.RegisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
      sigc::mem_fun(this, &DashView::OnActivateRequest));
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
      sigc::mem_fun(this, &DashView::OnBackgroundColorChanged));
}

long DashView::PostLayoutManagement (long LayoutResult)
{
  Relayout();
  return LayoutResult;
}

void DashView::Relayout()
{
  nux::Geometry geo = GetGeometry();
  content_geo_ = GetBestFitGeometry(geo);

  if (Settings::Instance().GetFormFactor() == FormFactor::NETBOOK)
  {
    if (geo.width >= content_geo_.width && geo.height > content_geo_.height)
      content_geo_ = geo;
  }

  // kinda hacky, but it makes sure the content isn't so big that it throws
  // the bottom of the dash off the screen
  // not hugely happy with this, so FIXME
  lenses_layout_->SetMaximumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height);
  lenses_layout_->SetMinimumHeight (content_geo_.height - search_bar_->GetGeometry().height - lens_bar_->GetGeometry().height);

  layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);

  dash::Style& style = dash::Style::Instance();

  // Minus the padding that gets added to the left
  float tile_width = style.GetTileWidth();
  style.SetDefaultNColumns(floorf((content_geo_.width - 32) / tile_width));

  ubus_manager_.SendMessage(UBUS_DASH_SIZE_CHANGED, g_variant_new("(ii)", content_geo_.width, content_geo_.height));

  QueueDraw();
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have uneccessary padding, everything will
// look tight
nux::Geometry DashView::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  dash::Style& style = dash::Style::Instance();

  int width = 0, height = 0;
  int tile_width = style.GetTileWidth();
  int tile_height = style.GetTileHeight();
  int half = for_geo.width / 2;

  // if default dash size is bigger than half a screens worth of items, go for that.
  while ((width += tile_width) + (19 * 2) < half)
    ;

  width = MAX(width, tile_width * 6);

  width += 19 + 32; // add the left padding and the group plugin padding

  height = search_bar_->GetGeometry().height;
  height += tile_height * 3;
  height += (24 + 15) * 3; // adding three group headers
  height += lens_bar_->GetGeometry().height;
  height += 6; // account for padding in PlacesGroup

  if (for_geo.width > 800 && for_geo.height > 550)
  {
    width = MIN(width, for_geo.width-66);
    height = MIN(height, for_geo.height-24);
  }

  return nux::Geometry(0, 0, width, height);
}

void DashView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  bool paint_blur = BackgroundEffectHelper::blur_type != BLUR_NONE;
  nux::Geometry geo = content_geo_;
  nux::Geometry geo_absolute = GetAbsoluteGeometry();

  if (Settings::Instance().GetFormFactor() != FormFactor::NETBOOK)
  {
    // Paint the edges
    {
      dash::Style& style = dash::Style::Instance();
      nux::BaseTexture* bottom = style.GetDashBottomTile();
      nux::BaseTexture* right = style.GetDashRightTile();
      nux::BaseTexture* corner = style.GetDashCorner();
      nux::BaseTexture* left_corner = style.GetDashLeftCorner();
      nux::BaseTexture* left_tile = style.GetDashLeftTile();
      nux::BaseTexture* top_corner = style.GetDashTopCorner();
      nux::BaseTexture* top_tile = style.GetDashTopTile();
      nux::TexCoordXForm texxform;

      int left_corner_offset = 10;
      int top_corner_offset = 10;

      geo = content_geo_;
      geo.width += corner->GetWidth() - 10;
      geo.height += corner->GetHeight() - 10;
      {
        // Corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x + (geo.width - corner->GetWidth()),
                            geo.y + (geo.height - corner->GetHeight()),
                            corner->GetWidth(),
                            corner->GetHeight(),
                            corner->GetDeviceTexture(),
                            texxform,
                            nux::color::White);
      }
      {
        // Bottom repeated texture
        int real_width = geo.width - (left_corner->GetWidth() - left_corner_offset) - corner->GetWidth();
        int offset = real_width % bottom->GetWidth();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(left_corner->GetWidth() - left_corner_offset - offset,
                            geo.y + (geo.height - bottom->GetHeight()),
                            real_width + offset,
                            bottom->GetHeight(),
                            bottom->GetDeviceTexture(),
                            texxform,
                            nux::color::White);
      }
      {
        // Bottom left corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x - left_corner_offset,
                            geo.y + (geo.height - left_corner->GetHeight()),
                            left_corner->GetWidth(),
                            left_corner->GetHeight(),
                            left_corner->GetDeviceTexture(),
                            texxform,
                            nux::color::White);
      }
      {
        // Left repeated texture
        nux::Geometry real_geo = GetGeometry();
        int real_height = real_geo.height - geo.height;
        int offset = real_height % left_tile->GetHeight();

        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x - 10,
                            geo.y + geo.height - offset,
                            left_tile->GetWidth(),
                            real_height + offset,
                            left_tile->GetDeviceTexture(),
                            texxform,
                            nux::color::White);
      }
      {
        // Right edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                             geo.y + top_corner->GetHeight() - top_corner_offset,
                             right->GetWidth(),
                             geo.height - corner->GetHeight() - (top_corner->GetHeight() - top_corner_offset),
                             right->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
      {
        // Top right corner
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_CLAMP_TO_BORDER, nux::TEXWRAP_CLAMP_TO_BORDER);

        gfx_context.QRP_1Tex(geo.x + geo.width - right->GetWidth(),
                            geo.y - top_corner_offset,
                            top_corner->GetWidth(),
                            top_corner->GetHeight(),
                            top_corner->GetDeviceTexture(),
                            texxform,
                            nux::color::White);
      }
      {
        // Top edge
        texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform.SetWrap(nux::TEXWRAP_REPEAT, nux::TEXWRAP_REPEAT);

        gfx_context.QRP_1Tex(geo.x + geo.width,
                             geo.y - 10,
                             GetGeometry().width - (geo.x + geo.width),
                             top_tile->GetHeight(),
                             top_tile->GetDeviceTexture(),
                             texxform,
                             nux::color::White);
      }
    }
  }

  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = ((float) content_geo_.x) / geo_absolute.width;
  texxform_absolute_bg.voffset = ((float) content_geo_.y) / geo_absolute.height;
  texxform_absolute_bg.SetWrap(TEXWRAP_CLAMP, TEXWRAP_CLAMP);

  if (paint_blur)
  {
    nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, content_geo_.width, content_geo_.height);
    bg_blur_texture_ = bg_effect_helper_.GetBlurRegion(blur_geo);

    if (bg_blur_texture_.IsValid()  && paint_blur)
    {
      nux::Geometry bg_clip = geo;
      gfx_context.PushClippingRectangle(bg_clip);

      gfx_context.GetRenderStates().SetBlend(false);
      gfx_context.QRP_1Tex (content_geo_.x, content_geo_.y,
                            content_geo_.width, content_geo_.height,
                            bg_blur_texture_, texxform_absolute_bg, color::White);
      gPainter.PopBackground();

      gfx_context.PopClippingRectangle();
    }
  }

  bg_darken_layer_->SetGeometry(content_geo_);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo_, bg_darken_layer_);

  bg_layer_->SetGeometry(content_geo_);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo_, bg_layer_);


  texxform_absolute_bg.flip_v_coord = false;
  texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * (GetAbsoluteGeometry().x);
  texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * (GetAbsoluteGeometry().y);

  gfx_context.GetRenderStates().SetColorMask(true, true, true, false);
  gfx_context.GetRenderStates().SetBlend(true, GL_DST_COLOR, GL_ONE);

  gfx_context.QRP_1Tex (content_geo_.x, content_geo_.y,
                        content_geo_.width, content_geo_.height,
                        bg_shine_texture_, texxform_absolute_bg, color::White);


  // Make round corners
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_ALPHA;
  nux::GetPainter().PaintShapeCornerROP(gfx_context,
                               content_geo_,
                               nux::color::White,
                               nux::eSHAPE_CORNER_ROUND4,
                               nux::eCornerBottomRight,
                               true,
                               rop);

  gfx_context.GetRenderStates().SetColorMask(true, true, true, true);
  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  geo = GetGeometry();
  nux::GetPainter().Paint2DQuadColor(gfx_context,
                                     nux::Geometry(geo.x,
                                                   geo.y,
                                                   1,
                                                   content_geo_.height + 5),
                                     nux::Color(0.0f, 0.0f, 0.0f, 0.0f),
                                     nux::Color(0.15f, 0.15f, 0.15f, 0.15f),
                                     nux::Color(0.15f, 0.15f, 0.15f, 0.15f),
                                     nux::Color(0.0f, 0.0f, 0.0f, 0.0f));
  nux::GetPainter().Paint2DQuadColor(gfx_context,
                                     nux::Geometry(geo.x,
                                                   geo.y,
                                                   content_geo_.width + 5,
                                                   1),
                                     nux::Color(0.0f, 0.0f, 0.0f, 0.0f),
                                     nux::Color(0.0f, 0.0f, 0.0f, 0.0f),
                                     nux::Color(0.15f, 0.15f, 0.15f, 0.15f),
                                     nux::Color(0.15f, 0.15f, 0.15f, 0.15f));

  geo = content_geo_;
  // Fill in corners (meh)
  for (int i = 1; i < 6; ++i)
  {
    nux::Geometry fill_geo (geo.x + geo.width, geo.y + i - 1, 6 - i, 1);
    nux::GetPainter().Paint2DQuadColor(gfx_context, fill_geo, bg_color_);

    nux::Color dark = bg_color_ * 0.8f;
    dark.alpha = bg_color_.alpha;
    fill_geo = nux::Geometry(geo.x + i - 1 , geo.y + geo.height, 1, 6 - i);
    nux::GetPainter().Paint2DQuadColor(gfx_context, fill_geo, dark);
  }
}

void DashView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  bool paint_blur = BackgroundEffectHelper::blur_type != BLUR_NONE;
  nux::Geometry geo = GetGeometry();
  int bgs = 0;

  gfx_context.PushClippingRectangle(geo);

  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  nux::Geometry geo_absolute = GetAbsoluteGeometry ();
  nux::TexCoordXForm texxform_absolute_bg;
  texxform_absolute_bg.flip_v_coord = true;
  texxform_absolute_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
  texxform_absolute_bg.uoffset = ((float) content_geo_.x) / geo_absolute.width;
  texxform_absolute_bg.voffset = ((float) content_geo_.y) / geo_absolute.height;
  texxform_absolute_bg.SetWrap(TEXWRAP_CLAMP, TEXWRAP_CLAMP);

  nux::ROPConfig rop;
  rop.Blend = false;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  if (bg_blur_texture_.IsValid() && paint_blur)
  {
    gPainter.PushTextureLayer(gfx_context, content_geo_,
                              bg_blur_texture_,
                              texxform_absolute_bg,
                              nux::color::White,
                              true, // write alpha?
                              rop);
    bgs++;
  }

  // draw the darkening behind our paint
  nux::GetPainter().PushLayer(gfx_context, bg_darken_layer_->GetGeometry(), bg_darken_layer_);
  bgs++;

  nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  bgs++;

  // apply the shine
  rop.Blend = true;
  rop.SrcBlend = GL_DST_COLOR;
  rop.DstBlend = GL_ONE;
  texxform_absolute_bg.flip_v_coord = false;
  texxform_absolute_bg.uoffset = (1.0f / bg_shine_texture_->GetWidth()) * (GetAbsoluteGeometry().x);
  texxform_absolute_bg.voffset = (1.0f / bg_shine_texture_->GetHeight()) * (GetAbsoluteGeometry().y);

  nux::GetPainter().PushTextureLayer(gfx_context, bg_layer_->GetGeometry(),
                                     bg_shine_texture_,
                                     texxform_absolute_bg,
                                     nux::color::White,
                                     false,
                                     rop);
  bgs++;

  if (IsFullRedraw())
  {
    nux::GetPainter().PushBackgroundStack();
    layout_->ProcessDraw(gfx_context, force_draw);
    nux::GetPainter().PopBackgroundStack();
  }
  else
  {
    layout_->ProcessDraw(gfx_context, force_draw);
  }

  nux::GetPainter().PopBackground(bgs);

  gfx_context.GetRenderStates().SetBlend(false);
  gfx_context.PopClippingRectangle();

  // Make round corners
  rop.Blend = true;
  rop.SrcBlend = GL_ZERO;
  rop.DstBlend = GL_SRC_ALPHA;
  nux::GetPainter().PaintShapeCornerROP(gfx_context,
                               content_geo_,
                               nux::color::White,
                               nux::eSHAPE_CORNER_ROUND4,
                               nux::eCornerBottomRight,
                               true,
                               rop);
}

void DashView::OnMouseButtonDown(int x, int y, unsigned long button, unsigned long key)
{
  if (!content_geo_.IsPointInside(x, y))
  {
    ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
  }
}

void DashView::OnActivateRequest(GVariant* args)
{
  glib::String uri;
  glib::String search_string;

  g_variant_get(args, "(sus)", &uri, NULL, &search_string);

  std::string id = AnalyseLensURI(uri.Str());

  home_view_->search_string = "";
  lens_bar_->Activate(id);


  if (id == "home.lens" || !visible_)
    ubus_manager_.SendMessage(UBUS_DASH_EXTERNAL_ACTIVATION);
}

std::string DashView::AnalyseLensURI(std::string uri)
{
  std::string id = uri;
  std::size_t pos = uri.find("?");

  // it's a real URI (with parameters)
  if (pos != std::string::npos)
  {
    // id is the uri from begining to the '?' position
    id = uri.substr(0, pos);

    // the components are from '?' position to the end
    std::string components = uri.substr(++pos);

    // split components in tokens
    std::vector<std::string> tokens;
    boost::split(tokens, components, boost::is_any_of("&"));

    for (std::string const& token : tokens)
    {
      // split each token in a pair
      std::vector<std::string> subs;
      boost::split(subs, token, boost::is_any_of("="));

      // check if it's a filter
      if (boost::starts_with(subs[0], "filter_"))
      {
          UpdateLensFilter(id, subs[0].substr(7), subs[1]);
          lens_views_[id]->filters_expanded = true;
      }
    }
  }

  return id;
}

void DashView::UpdateLensFilter(std::string lens_id, std::string filter_name, std::string value)
{
  if (lenses_.GetLens(lens_id))
  {
    Lens::Ptr lens = lenses_.GetLens(lens_id);

    Filters::Ptr filters = lens->filters;

    for (unsigned int i = 0; i < filters->count(); ++i)
    {
      Filter::Ptr filter = filters->FilterAtIndex(i);

      if (filter->id() == filter_name)
      {
        UpdateLensFilterValue(filter, value);
      }
    }
  }
}

void DashView::UpdateLensFilterValue(Filter::Ptr filter, std::string value)
{
  if (filter->renderer_name == "filter-radiooption")
  {
    RadioOptionFilter::Ptr radio = std::static_pointer_cast<RadioOptionFilter>(filter);
    for (auto option: radio->options())
    {
      if (option->id == value)
        option->active = true;
    }
  }
}

void DashView::OnBackgroundColorChanged(GVariant* args)
{
  gdouble red, green, blue, alpha;
  g_variant_get (args, "(dddd)", &red, &green, &blue, &alpha);

  nux::Color color = nux::Color(red, green, blue, alpha);
  bg_layer_->SetColor(color);
  bg_color_ = color;
  QueueDraw();
}

void DashView::OnSearchChanged(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Search changed: " << search_string;
}

void DashView::OnLiveSearchReached(std::string const& search_string)
{
  LOG_DEBUG(logger) << "Live search reached: " << search_string;
  if (active_lens_view_)
  {
    active_lens_view_->search_string = search_string;
  }
}

void DashView::OnLensAdded(Lens::Ptr& lens)
{
  std::string id = lens->id;
  lens_bar_->AddLens(lens);
  home_view_->AddLens(lens);

  LensView* view = new LensView(lens);
  view->SetVisible(false);
  view->uri_activated.connect(sigc::mem_fun(this, &DashView::OnUriActivated));
  lenses_layout_->AddView(view, 1);
  lens_views_[lens->id] = view;

  lens->activated.connect(sigc::mem_fun(this, &DashView::OnUriActivatedReply));
  lens->search_finished.connect(sigc::mem_fun(this, &DashView::OnSearchFinished));
  lens->global_search_finished.connect(sigc::mem_fun(this, &DashView::OnGlobalSearchFinished));
}

void DashView::OnLensBarActivated(std::string const& id)
{
  if (lens_views_.find(id) == lens_views_.end())
  {
    LOG_WARN(logger) << "Unable to find Lens " << id;
    return;
  }

  for (auto it: lens_views_)
  {
    it.second->SetVisible(it.first == id);
    it.second->active = it.first == id;
  }

  LensView* view = active_lens_view_ = lens_views_[id];
  search_bar_->search_string = view->search_string;
  if (view != home_view_)
    search_bar_->search_hint = view->lens()->search_hint;
  else
    search_bar_->search_hint = _("Search");
  bool expanded = view->filters_expanded;
  search_bar_->showing_filters = expanded;

  search_bar_->text_entry()->SelectAll();
  nux::GetWindowCompositor().SetKeyFocusArea(search_bar_->text_entry());

  search_bar_->can_refine_search = view->can_refine_search();

  view->QueueDraw();
  QueueDraw();
}

void DashView::OnSearchFinished(std::string const& search_string)
{
  if (search_bar_->search_string == search_string)
    search_bar_->SearchFinished();
}

void DashView::OnGlobalSearchFinished(std::string const& search_string)
{
  if (active_lens_view_ == home_view_)
    OnSearchFinished(search_string);
}

void DashView::OnUriActivated(std::string const& uri)
{
  last_activated_uri_ = uri;
}

void DashView::OnUriActivatedReply(std::string const& uri, HandledType type, Lens::Hints const&)
{
  // We don't want to close the dash if there was another activation pending
  if (type == NOT_HANDLED)
  {
    if (!DoFallbackActivation(uri))
      return;
  }
  else if (type == SHOW_DASH)
  {
    return;
  }

  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

bool DashView::DoFallbackActivation(std::string const& fake_uri)
{
  size_t pos = fake_uri.find(":");
  std::string uri = fake_uri.substr(++pos);

  LOG_DEBUG(logger) << "Fallback activating " << uri;

  if (g_str_has_prefix(uri.c_str(), "application://"))
  {
    std::string appname = uri.substr(14);
    return LaunchApp(appname);
  }
  else if (g_str_has_prefix(uri.c_str(), "unity-runner://"))
  {
    std::string appname = uri.substr(15);
    return LaunchApp(appname);
  }
  else
    return gtk_show_uri(NULL, uri.c_str(), time(NULL), NULL);

  return false;
}

bool DashView::LaunchApp(std::string const& appname)
{
  GDesktopAppInfo* info;
  bool ret = false;
  char *id = g_strdup(appname.c_str());
  int i = 0;

  LOG_DEBUG(logger) << "Launching " << appname;

  while (id != NULL)
  {
    info = g_desktop_app_info_new(id);
    if (info != NULL)
    {
      GError* error = NULL;

      g_app_info_launch(G_APP_INFO(info), NULL, NULL, &error);
      if (error)
      {
        g_warning("Unable to launch %s: %s", id,  error->message);
        g_error_free(error);
      }
      else
        ret = true;
      g_object_unref(info);
      break;
    }

    /* Try to replace the next - with a / and do the lookup again.
     * If we set id=NULL we'll exit the outer loop */
    for (i = 0; ; i++)
    {
      if (id[i] == '-')
      {
        id[i] = '/';
        break;
      }
      else if (id[i] == '\0')
      {
        g_free(id);
        id = NULL;
        break;
      }
    }
  }

  g_free(id);
  return ret;
}

void DashView::DisableBlur()
{
  bg_effect_helper_.blur_type = BLUR_NONE;
}
void DashView::OnEntryActivated()
{
  active_lens_view_->ActivateFirst();
}

// Keyboard navigation
bool DashView::AcceptKeyNavFocus()
{
  return false;
}

std::string const DashView::GetIdForShortcutActivation(std::string const& shortcut) const
{
  Lens::Ptr lens = lenses_.GetLensForShortcut(shortcut);
  if (lens)
    return lens->id;
  return "";
}

std::vector<char> DashView::GetAllShortcuts()
{
  std::vector<char> result;

  for (Lens::Ptr lens: lenses_.GetLenses())
  {
    std::string shortcut = lens->shortcut;
    if(shortcut.size() > 0)
      result.push_back(shortcut.at(0));
  }
  return result;
}

bool DashView::InspectKeyEvent(unsigned int eventType,
                               unsigned int key_sym,
                               const char* character)
{
  if ((eventType == nux::NUX_KEYDOWN) && (key_sym == NUX_VK_ESCAPE))
  {
    if (search_bar_->search_string == "")
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
    else
      search_bar_->search_string = "";
    return true;
  }
  return false;
}

nux::View* DashView::default_focus() const
{
  return search_bar_->text_entry();
}

// Introspectable
const gchar* DashView::GetName()
{
  return "DashView";
}

void DashView::AddProperties(GVariantBuilder* builder)
{}

nux::Area * DashView::KeyNavIteration(nux::KeyNavDirection direction)
{
  // We don't want to eat the tab as it's used for IM stuff
  if (!search_bar_->im_active())
  {
    if (direction == KEY_NAV_TAB_NEXT)
      lens_bar_->ActivateNext();
    else if (direction == KEY_NAV_TAB_PREVIOUS)
      lens_bar_->ActivatePrevious();
  }
  return this;
}

void DashView::ProcessDndEnter()
{
  ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
}

Area* DashView::FindKeyFocusArea(unsigned int key_symbol,
      unsigned long x11_key_code,
      unsigned long special_keys_state)
{
  // Do what nux::View does, but if the event isn't a key navigation,
  // designate the text entry to process it.

  nux::KeyNavDirection direction = KEY_NAV_NONE;
  switch (x11_key_code)
  {
  case NUX_VK_UP:
    direction = KEY_NAV_UP;
    break;
  case NUX_VK_DOWN:
    direction = KEY_NAV_DOWN;
    break;
  case NUX_VK_LEFT:
    direction = KEY_NAV_LEFT;
    break;
  case NUX_VK_RIGHT:
    direction = KEY_NAV_RIGHT;
    break;
  case NUX_VK_LEFT_TAB:
    direction = KEY_NAV_TAB_PREVIOUS;
    break;
  case NUX_VK_TAB:
    direction = KEY_NAV_TAB_NEXT;
    break;
  case NUX_VK_ENTER:
  case NUX_KP_ENTER:
    // Not sure if Enter should be a navigation key
    direction = KEY_NAV_ENTER;
    break;
  default:
    direction = KEY_NAV_NONE;
    break;
  }

  if (has_key_focus_)
  {
    return this;
  }
  else if (direction == KEY_NAV_NONE || search_bar_->im_active)
  {
    // then send the event to the search entry
    return search_bar_->text_entry();
  }
  else if (next_object_to_key_focus_area_)
  {
    return next_object_to_key_focus_area_->FindKeyFocusArea(key_symbol, x11_key_code, special_keys_state);
  }
  return NULL;
}

}
}
