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
#include <Nux/TextureArea.h>
#include <Nux/VLayout.h>
#include <NuxCore/Logger.h>
#include <NuxImage/CairoGraphics.h>

#include "DashStyle.h"
#include "FilterBar.h"
#include "FilterFactory.h"

namespace unity
{
namespace dash
{
namespace
{

nux::logging::Logger logger("unity.dash.filterbar");

}

NUX_IMPLEMENT_OBJECT_TYPE(FilterBar);

FilterBar::FilterBar(NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
{
  // TODO - does the filterbar associate itself with a model of some sort?
  // does libunity provide a Lens.Filters model or something that we can update on?
  // don't want to associate a Filterbar with just a lens model, its a filter bar not a
  // lens parser
  Init();
}

FilterBar::~FilterBar()
{
}

void FilterBar::Init()
{
  nux::LinearLayout* layout = new nux::VLayout(NUX_TRACKER_LOCATION);
  layout->SetSpaceBetweenChildren(10);
  SetLayout(layout);

  nux::CairoGraphics cairo(CAIRO_FORMAT_ARGB32, 295, 3);
  cairo_t* cr = cairo.GetContext();
  Style::Instance().SeparatorHoriz(cr);
  cairo_surface_write_to_png (cairo_get_target(cr), "/tmp/separator.png");
  cairo_destroy (cr);
  nux::NBitmapData* bitmap = cairo.GetBitmap();
  separator_ = new nux::Texture2D();
  separator_->Update(bitmap);
}

void FilterBar::SetFilters(Filters::Ptr const& filters)
{
  filters_ = filters;
}

void FilterBar::AddFilter(Filter::Ptr const& filter)
{
  if (filter_map_.count(filter) > 0)
  {
    LOG_WARN(logger) << "Attempting to add a filter that has already been added";
    return;
  }

  if (filter_map_.size() >= 1)
  {
    nux::TextureArea* texture = new nux::TextureArea(NUX_TRACKER_LOCATION);
	texture->SetTexture(separator_);
    texture->SetMinimumSize(separator_->GetWidth(), separator_->GetHeight());
    GetLayout()->AddView(texture, 0, nux::MINOR_POSITION_CENTER);
  }

  nux::View* filter_view = factory_.WidgetForFilter(filter);
  filter_map_[filter] = filter_view;
  GetLayout()->AddView(filter_view, 0, nux::MINOR_POSITION_LEFT, nux::MINOR_SIZE_FULL);
}

void FilterBar::RemoveFilter(Filter::Ptr const& filter)
{
  for (auto iter: filter_map_)
  {
    if (iter.first->id == filter->id)
    {
      nux::View* filter_view = iter.second;
      filter_map_.erase(filter_map_.find(iter.first));
      GetLayout()->RemoveChildObject(filter_view);
      break;
    }
  }
}

void FilterBar::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void FilterBar::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  GetLayout()->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

} // namespace dash
} // namespace unity

