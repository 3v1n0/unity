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
{
  SetupBackground();
  SetupViews();
  SetupUBusConnections();
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

  search_bar_ = new SearchBar();
  layout_->AddView(search_bar_, 0, nux::MINOR_POSITION_CENTER);
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

long DashView::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  return traverse_info;
}

void DashView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  bg_layer_->SetGeometry(GetGeometry());
  nux::GetPainter().RenderSinglePaintLayer(gfx_context, GetGeometry(), bg_layer_);
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
    //FIXME:SetActiveEntry(NULL, 0, "");
    return true;
  }
  return false;
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
