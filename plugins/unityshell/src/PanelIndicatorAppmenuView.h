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

#ifndef PANEL_INDICATOR_APPMENU_VIEW_H
#define PANEL_INDICATOR_APPMENU_VIEW_H

#include <Nux/Nux.h>
#include <Nux/BaseWindow.h>

#include "PanelIndicatorEntryView.h"

namespace unity
{
using indicator::Entry;

class PanelIndicatorAppmenuView : public PanelIndicatorEntryView
{
public:
  PanelIndicatorAppmenuView(Entry::Ptr const& proxy);

  std::string GetLabel();
  void SetLabel(std::string const& label);
  bool IsLabelVisible() const;

  void SetControlledWindowXid(Window xid);

  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  void Activate();

protected:
  void ShowMenu(int button);
  void DrawEntryPrelight(cairo_t* cr, unsigned int w, unsigned int h);

private:
  Window xid_;
  std::string label_;
};

}

#endif // PANEL_INDICATOR_APPMENU_VIEW_H
