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

#include "Introspectable.h"

#define PANEL_HEIGHT 24
#define PADDING 6
#define SPACING 3

class PanelIndicatorObjectEntryView : public nux::TextureArea, public Introspectable
{
public:
  PanelIndicatorObjectEntryView (IndicatorObjectEntryProxy *proxy);
  ~PanelIndicatorObjectEntryView ();

  void Refresh ();
  void OnMouseDown (int x, int y, long button_flags, long key_flags);
  void Activate ();
  void OnActiveChanged (bool is_active);

  const gchar * GetName ();
  void          AddProperties (GVariantBuilder *builder);

  sigc::signal<void, PanelIndicatorObjectEntryView *, bool> active_changed;

public:
  IndicatorObjectEntryProxy *_proxy;
private:
  nux::CairoGraphics _util_cg;
};

#endif // PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H
