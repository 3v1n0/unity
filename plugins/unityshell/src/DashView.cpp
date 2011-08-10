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

#include <NuxCore/Logger.h>
#include <UnityCore/GLibWrapper.h>

#include "PlacesStyle.h"
#include "PlacesSettings.h"
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
  , size_mode_(SIZE_MODE_NORMAL)
  , active_lens_view_(0)

{
  SetupBackground();
  SetupViews();
  SetupUBusConnections();

  lenses_.lens_added.connect(sigc::mem_fun(this, &DashView::OnLensAdded));

  Relayout();
}

DashView::~DashView()
{}

void DashView::AboutToShow()
{}

void DashView::SetupBackground()
{
  nux::ROPConfig rop;
  rop.Blend = true;
  rop.SrcBlend = GL_ONE;
  rop.DstBlend = GL_ONE_MINUS_SRC_ALPHA;
  bg_layer_ = new nux::ColorLayer(nux::Color(0.0f, 0.0f, 0.0f, 0.9), true, rop);

  ubus_manager_.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT);
}

void DashView::SetupViews()
{
  layout_ = new nux::VLayout();
  SetLayout(layout_);

  content_layout_ = new nux::VLayout();
  layout_->AddLayout(content_layout_, 1, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FIX); 
  search_bar_ = new SearchBar();
  search_bar_->search_changed.connect(sigc::mem_fun(this, &DashView::OnSearchChanged));
  search_bar_->live_search_reached.connect(sigc::mem_fun(this, &DashView::OnLiveSearchReached));
  content_layout_->AddView(search_bar_, 0, nux::MINOR_POSITION_LEFT);

  lenses_layout_ = new nux::VLayout();
  content_layout_->AddView(lenses_layout_, 1, nux::MINOR_POSITION_LEFT);

  lens_bar_ = new LensBar();
  lens_bar_->lens_activated.connect(sigc::mem_fun(this, &DashView::OnLensBarActivated));
  content_layout_->AddView(lens_bar_, 0, nux::MINOR_POSITION_CENTER);
}

void DashView::SetupUBusConnections()
{
  ubus_manager_.RegisterInterest(UBUS_PLACE_ENTRY_ACTIVATE_REQUEST,
      sigc::mem_fun(this, &DashView::OnActivateRequest));
  ubus_manager_.RegisterInterest(UBUS_PLACE_VIEW_QUEUE_DRAW,
      [&] (GVariant* args) { QueueDraw(); });
  ubus_manager_.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED,
      sigc::mem_fun(this, &DashView::OnBackgroundColorChanged));
}

void DashView::Relayout()
{
  nux::Geometry geo = GetGeometry();
  nux::Geometry best_geo = GetBestFitGeometry(geo);
    
  if (size_mode_ == SIZE_MODE_MAXIMISED)
  {
    content_geo_ = geo;
  }
  else if (size_mode_ == SIZE_MODE_NORMAL)
  {
    content_geo_ = best_geo;
  }
  else if (size_mode_ == SIZE_MODE_VERTICAL_MAXIMISED)
  {
    content_geo_ = geo;
    content_geo_.width = best_geo.width;
  }
  else //size_mode_ == SIZE_MODE_HORIZONATAL_MAXIMISED
  {
    content_geo_ = geo;
    content_geo_.height = best_geo.height;
  }
 
  // FIXME: Remove edges
  content_layout_->SetMinMaxSize(content_geo_.width, content_geo_.height);
}

// Gives us the width and height of the contents that will give us the best "fit",
// which means that the icons/views will not have uneccessary padding, everything will
// look tight
nux::Geometry DashView::GetBestFitGeometry(nux::Geometry const& for_geo)
{
  PlacesStyle* style = PlacesStyle::GetDefault();

  int width = 0, height = 0;
  int tile_width = style->GetTileWidth();
  int tile_height = style->GetTileHeight();
  int half = for_geo.width / 2;
  
  while ((width += tile_width) < half)
    ;

  width = MAX(width, tile_width * 7);

  height = search_bar_->GetGeometry().height;
  height += tile_height * 5;
  height += lens_bar_->GetGeometry().height;

  return nux::Geometry(0, 0, width, height);
}

long DashView::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  long ret = traverse_info;

  if ((ievent.e_event == nux::NUX_KEYDOWN) &&
      (ievent.GetKeySym() == NUX_VK_ESCAPE))
  {
    if (search_bar_->search_string == "")
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
    else
      search_bar_->search_string = "";
    return ret;
  }

  if (ievent.e_event == nux::NUX_MOUSE_PRESSED)
  {
    if (!content_geo_.IsPointInside(ievent.e_x, ievent.e_y))
    {
      ubus_manager_.SendMessage(UBUS_PLACE_VIEW_CLOSE_REQUEST);
      return ret |= nux::eMouseEventSolved;
    }
  }

  ret = layout_->ProcessEvent(ievent, traverse_info, event_info);
  return ret;
}

void DashView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  bg_layer_->SetGeometry(content_geo_);
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, content_geo_, bg_layer_);
}

void DashView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry clip_geo = GetGeometry();

  clip_geo.height = bg_layer_->GetGeometry().height - 1;
  gfx_context.PushClippingRectangle(clip_geo);

  gfx_context.GetRenderStates().SetBlend(true);
  gfx_context.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  nux::GetPainter().PushLayer(gfx_context, bg_layer_->GetGeometry(), bg_layer_);
  layout_->ProcessDraw(gfx_context, force_draw);
  nux::GetPainter().PopBackground();

  gfx_context.GetRenderStates().SetBlend(false);
  gfx_context.PopClippingRectangle();
}

void DashView::OnActivateRequest(GVariant* args)
{
  glib::String id;
  glib::String search_string;

  g_variant_get(args, "(sus)", &id, NULL, &search_string);

  LOG_WARN(logger) << "ActivateRequest not supported yet: "
                   << id << "::" << search_string;

  // Reset focus
  SetFocused(false);
  SetFocused(true);
}

void DashView::OnBackgroundColorChanged(GVariant* args)
{
  gdouble red, green, blue, alpha;
  g_variant_get (args, "(dddd)", &red, &green, &blue, &alpha);

  nux::Color color = nux::Color(red, green, blue, alpha);
  bg_layer_->SetColor(color);
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
  lens_bar_->AddLens(lens);

  LensView* view = new LensView(lens);
  view->SetVisible(false);
  lenses_layout_->AddView(view, 1);
  lens_views_[lens->id] = view;
}

void DashView::OnLensBarActivated(std::string const& id)
{
  for (auto it: lens_views_)
    it.second->SetVisible(it.first == id);

  LensView* view = active_lens_view_ = lens_views_[id];
  search_bar_->search_string = view->search_string;
  search_bar_->search_hint = view->lens()->search_hint;
}

// Keyboard navigation
bool DashView::AcceptKeyNavFocus()
{
  return false;
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


}
}
