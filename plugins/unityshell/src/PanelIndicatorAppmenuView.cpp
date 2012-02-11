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

namespace unity
{  
using indicator::Entry;

PanelIndicatorAppmenuView::PanelIndicatorAppmenuView(Entry::Ptr const& proxy)
  : PanelIndicatorEntryView(proxy, 0, APPMENU)
  , xid_(0)
{}

void PanelIndicatorAppmenuView::ShowMenu(int button)
{
  if (xid_)
  {
    WindowManager::Default()->Activate(xid_);

    proxy_->ShowMenu(xid_,
                     GetAbsoluteX(),
                     GetAbsoluteY() + PANEL_HEIGHT,
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

void PanelIndicatorAppmenuView::DrawEntryBackground(cairo_t* cr, unsigned int width, unsigned int height)
{
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
