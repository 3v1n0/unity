
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

#include "Nux/Nux.h"
#include "Nux/Area.h"
#include "Nux/HLayout.h"
#include "Nux/VLayout.h"

#include "NuxCore/Logger.h"

#include "NuxGraphics/GLThread.h"
#include "Nux/BaseWindow.h"
#include "Nux/WindowCompositor.h"

#include "PanelIndicatorObjectView.h"

#include <glib.h>

namespace
{
nux::logging::Logger logger("unity.indicators");
}

namespace unity
{

PanelIndicatorObjectView::PanelIndicatorObjectView()
  : View(NUX_TRACKER_LOCATION)
  , layout_(NULL)
{
}

PanelIndicatorObjectView::PanelIndicatorObjectView(indicator::Indicator::Ptr const& proxy)
  : View(NUX_TRACKER_LOCATION)
  , proxy_(proxy)
{
  LOG_DEBUG(logger) << "IndicatorAdded: " << proxy_->name();
  layout_ = new nux::HLayout("", NUX_TRACKER_LOCATION);

  SetCompositionLayout(layout_);

  // default in Nux is 32, we have some PanelIndicatorObjectEntryView which are smaller than that.
  // so redefining the minimum value for them.
  SetMinimumWidth(MINIMUM_INDICATOR_WIDTH);

  on_entry_added_connection_ = proxy_->on_entry_added.connect(sigc::mem_fun(this, &PanelIndicatorObjectView::OnEntryAdded));
}

PanelIndicatorObjectView::~PanelIndicatorObjectView()
{
  on_entry_added_connection_.disconnect();
}

long
PanelIndicatorObjectView::ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo)
{
  long ret = TraverseInfo;

  if (layout_)
    ret = layout_->ProcessEvent(ievent, ret, ProcessEventInfo);
  return ret;
}

void
PanelIndicatorObjectView::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry geo = GetGeometry();

  GfxContext.PushClippingRectangle(geo);
  nux::GetPainter().PaintBackground(GfxContext, geo);
  GfxContext.PopClippingRectangle();
}

void PanelIndicatorObjectView::QueueDraw()
{
  nux::View::QueueDraw();
  for (Entries::iterator i = entries_.begin(), end = entries_.end(); i != end; ++i)
  {
    (*i)->QueueDraw();
  }
}

bool PanelIndicatorObjectView::ActivateEntry(std::string const& entry_id)
{
  for (Entries::iterator i = entries_.begin(), end = entries_.end();
       i != end; ++i)
  {
    PanelIndicatorObjectEntryView* view = *i;
    if (view->IsEntryValid() && entry_id == view->GetName())
    {
      LOG_DEBUG(logger) << "Activating: " << entry_id;
      view->Activate();
      return true;
    }
  }
  return false;
}

bool PanelIndicatorObjectView::ActivateIfSensitive()
{
  for (Entries::iterator i = entries_.begin(), end = entries_.end();
       i != end; ++i)
  {
    PanelIndicatorObjectEntryView* view = *i;
    if (view->IsSensitive())
    {
      view->Activate();
      return true;
    }
  }
  return false;
}

void PanelIndicatorObjectView::GetGeometryForSync(indicator::EntryLocationMap& locations)
{
  for (Entries::iterator i = entries_.begin(), end = entries_.end();
       i != end; ++i)
  {
    (*i)->GetGeometryForSync(locations);
  }
}

void PanelIndicatorObjectView::OnPointerMoved(int x, int y)
{
  for (Entries::iterator i = entries_.begin(), end = entries_.end();
       i != end; ++i)
  {
    PanelIndicatorObjectEntryView* view = *i;

    nux::Geometry geo = view->GetAbsoluteGeometry();
    if (geo.IsPointInside(x, y))
    {
      view->OnMouseDown(x, y, 0, 0);
      break;
    }
  }
}

void
PanelIndicatorObjectView::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  GfxContext.PushClippingRectangle(GetGeometry());
  if (layout_)
    layout_->ProcessDraw(GfxContext, force_draw);
  GfxContext.PopClippingRectangle();
}

void PanelIndicatorObjectView::OnEntryAdded(indicator::Entry::Ptr const& proxy)
{
  PanelIndicatorObjectEntryView* view = new PanelIndicatorObjectEntryView(proxy);
  layout_->AddView(view, 0, nux::eCenter, nux::eFull);
  layout_->SetContentDistribution(nux::eStackRight);

  entries_.push_back(view);

  AddChild(view);

  QueueRelayout();
  QueueDraw();
}

const gchar* PanelIndicatorObjectView::GetName()
{
  return proxy_->name().c_str();
}

const gchar* PanelIndicatorObjectView::GetChildsName()
{
  return "entries";
}

void
PanelIndicatorObjectView::AddProperties(GVariantBuilder* builder)
{
  nux::Geometry geo = GetGeometry();

  /* Now some props from ourselves */
  g_variant_builder_add(builder, "{sv}", "x", g_variant_new_int32(geo.x));
  g_variant_builder_add(builder, "{sv}", "y", g_variant_new_int32(geo.y));
  g_variant_builder_add(builder, "{sv}", "width", g_variant_new_int32(geo.width));
  g_variant_builder_add(builder, "{sv}", "height", g_variant_new_int32(geo.height));
}

} // namespace unity
