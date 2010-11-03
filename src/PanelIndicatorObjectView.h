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
#include "PanelIndicatorObjectEntryView.h"

class PanelIndicatorObjectView : public nux::View
{
public:
  PanelIndicatorObjectView (IndicatorObjectProxy *proxy);
  ~PanelIndicatorObjectView ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsContext& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsContext &GfxContext, bool force_draw);

  void OnEntryAdded (IndicatorObjectEntryProxy *proxy);
  void OnEntryMoved (IndicatorObjectEntryProxy *proxy);
  void OnEntryRemoved (IndicatorObjectEntryProxy *proxy);

private:
  nux::HLayout *_layout;
  IndicatorObjectProxy *_proxy;
  std::vector<PanelIndicatorObjectEntryView *> _entries;
};

#endif // PANEL_INDICATOR_OBJECT_VIEW_H
