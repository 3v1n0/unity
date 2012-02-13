// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2012 Canonical Ltd
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
 * Authored by: Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 */

#include <UnityCore/Variant.h>

#include "PanelIndicatorAppmenuView.h"
#include "WindowManager.h"
#include "PanelStyle.h"

namespace unity
{  
using indicator::Entry;

PanelIndicatorAppmenuView::PanelIndicatorAppmenuView(Entry::Ptr const& proxy)
  : PanelIndicatorEntryView(proxy, 0, APPMENU)
  , xid_(0)
{}

void PanelIndicatorAppmenuView::Activate()
{
  ShowMenu(1);
}

void PanelIndicatorAppmenuView::ShowMenu(int button)
{
  if (xid_)
  {
    WindowManager::Default()->Activate(xid_);

    proxy_->ShowMenu(xid_,
                     GetAbsoluteX(),
                     GetAbsoluteY() + panel::Style::Instance().panel_height,
                     1,
                     time(nullptr));
  }
}

std::string PanelIndicatorAppmenuView::GetLabel()
{
  glib::String escaped(g_markup_escape_text(label_.c_str(), -1));

  std::ostringstream bold_label;
  bold_label << "<b>" << escaped.Str() << "</b>";

  return bold_label.str();
}

bool PanelIndicatorAppmenuView::IsLabelVisible() const
{
  return !label_.empty();
}

void PanelIndicatorAppmenuView::SetLabel(std::string const& label)
{
  if (label_ != label)
  {
    label_ = label;
    Refresh();
  }
}

void PanelIndicatorAppmenuView::SetControlledWindowXid(Window xid)
{
  xid_ = xid;
}

void PanelIndicatorAppmenuView::DrawEntryPrelight(cairo_t* cr, unsigned int width, unsigned int height)
{
  nux::Rect const& geo = proxy_->geometry();

  cairo_surface_t *left = cairo_image_surface_create_from_png(PKGDATADIR "/lim_flair_left.png");
  cairo_surface_t *center = cairo_image_surface_create_from_png(PKGDATADIR "/lim_flair_center.png");
  cairo_surface_t *right = cairo_image_surface_create_from_png(PKGDATADIR "/lim_flair_right.png");

  int left_w = cairo_image_surface_get_width(left);
  int right_w = cairo_image_surface_get_width(right);
  int center_h = cairo_image_surface_get_height(center);
  int center_w = geo.width - left_w - right_w;

  int x = geo.x - GetAbsoluteX();
  int y = geo.y - center_h;

  cairo_set_source_surface(cr, left, x, y);
  cairo_paint(cr);

  cairo_pattern_t* repeat_bg = cairo_pattern_create_for_surface(center);
  cairo_pattern_set_extend(repeat_bg, CAIRO_EXTEND_REPEAT);
  cairo_set_source(cr, repeat_bg);

  x += left_w;
  cairo_rectangle(cr, x, y, center_w, center_h);
  cairo_fill(cr);

  x += center_w;
  cairo_set_source_surface(cr, right, x, y);
  cairo_paint(cr);

  cairo_surface_destroy(left);
  cairo_surface_destroy(center);
  cairo_surface_destroy(right);
  cairo_pattern_destroy(repeat_bg);
}

std::string PanelIndicatorAppmenuView::GetName() const
{
  return "appmenu";
}

void PanelIndicatorAppmenuView::AddProperties(GVariantBuilder* builder)
{
  PanelIndicatorEntryView::AddProperties(builder);

  variant::BuilderWrapper(builder)
  .add("controlled-window", xid_);
}

} // NAMESPACE
