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

#ifndef PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H
#define PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "IndicatorObjectEntryProxy.h"

class PanelIndicatorObjectEntryView : public nux::TextureArea
{
public:
  PanelIndicatorObjectEntryView (IndicatorObjectEntryProxy *proxy);
  ~PanelIndicatorObjectEntryView ();

private:
  void Refresh ();
  void OnMouseDown (int x, int y, long button_flags, long key_flags);

public:
  IndicatorObjectEntryProxy *_proxy;
private:
  nux::CairoGraphics _util_cg;
};

#endif // PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H
