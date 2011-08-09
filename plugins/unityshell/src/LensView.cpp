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

#include "LensView.h"

#include <NuxCore/Logger.h>

#include "ResultRendererTile.h"

namespace unity
{
namespace dash
{

namespace
{
nux::logging::Logger logger("unity.dash.lensview");
}

NUX_IMPLEMENT_OBJECT_TYPE(LensView);

LensView::LensView(Lens::Ptr lens)
  : nux::View(NUX_TRACKER_LOCATION)
  , lens_(lens)
{
  SetupViews();
}

LensView::~LensView()
{}

void LensView::SetupViews()
{
  result_view_ = new ResultViewGrid(NUX_TRACKER_LOCATION);
  result_view_->SetModelRenderer(new ResultRendererTile(NUX_TRACKER_LOCATION));

  scroll_layout_ = new nux::VLayout();
  scroll_layout_->AddView(result_view_);

  scroll_view_ = new nux::ScrollView();
  scroll_view_->EnableVerticalScrollBar(true);
  scroll_view_->SetLayout(scroll_layout_);

  layout_ = new nux::HLayout();
  layout_->AddView(scroll_view_);

  SetLayout(layout_);
}

long LensView::ProcessEvent(nux::IEvent& ievent, long traverse_info, long event_info)
{
  return traverse_info;
}

void LensView::Draw(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  gfx_context.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(gfx_context, geo);
  gfx_context.PopClippingRectangle();
}

void LensView::DrawContent(nux::GraphicsEngine& gfx_context, bool force_draw)
{
  gfx_context.PushClippingRectangle(GetGeometry());

  //layout_->ProcessDraw(gfx_context, force_draw);

  gfx_context.PopClippingRectangle();
}

// Keyboard navigation
bool LensView::AcceptKeyNavFocus()
{
  return false;
}

// Introspectable
const gchar* LensView::GetName()
{
  return "LensView";
}

void LensView::AddProperties(GVariantBuilder* builder)
{}


}
}
