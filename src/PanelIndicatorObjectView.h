// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
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

#ifndef PANEL_INDICATOR_OBJECT_VIEW_H
#define PANEL_INDICATOR_OBJECT_VIEW_H

#include <Nux/View.h>

#include "IndicatorObjectProxy.h"
#include "IndicatorEntry.h"
#include "PanelIndicatorObjectEntryView.h"

#include "Introspectable.h"

#define MINIMUM_INDICATOR_WIDTH 12

class PanelIndicatorObjectView : public nux::View, public Introspectable
{
public:
  PanelIndicatorObjectView ();
  PanelIndicatorObjectView (IndicatorObjectProxy *proxy);
  ~PanelIndicatorObjectView ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void OnEntryAdded(unity::indicator::Entry::Ptr proxy);
  virtual void OnEntryMoved(unity::indicator::Entry::Ptr proxy);
  virtual void OnEntryRemoved(unity::indicator::Entry::Ptr proxy);

  nux::HLayout *_layout;

protected:
  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);

  IndicatorObjectProxy *_proxy;
  std::vector<PanelIndicatorObjectEntryView *> _entries;

private:
  sigc::connection _on_entry_added_connection;
  sigc::connection _on_entry_moved_connection;
  sigc::connection _on_entry_removed_connection;
};

#endif // PANEL_INDICATOR_OBJECT_VIEW_H
