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

#ifndef PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H
#define PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H

#include <Nux/TextureArea.h>
#include <Nux/View.h>
#include <NuxImage/CairoGraphics.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "IndicatorEntry.h"

#include "Introspectable.h"

#define PANEL_HEIGHT 24
#define SPACING 3

class PanelIndicatorObjectEntryView : public nux::TextureArea, public Introspectable
{
public:
  PanelIndicatorObjectEntryView(unity::indicator::Entry::Ptr const& proxy,
                                int padding = 3);
  ~PanelIndicatorObjectEntryView();

  void Refresh ();
  void OnMouseDown (int x, int y, long button_flags, long key_flags);
  void OnMouseWheel (int x, int y, int delta, unsigned long mouse_state, unsigned long key_state);
  void Activate ();
  void OnActiveChanged (bool is_active);
  bool GetShowNow ();

  const gchar * GetName ();
  void          AddProperties (GVariantBuilder *builder);

  sigc::signal<void, PanelIndicatorObjectEntryView *, bool> active_changed;
  sigc::signal<void, PanelIndicatorObjectEntryView *> refreshed;

public:
  unity::indicator::Entry::Ptr _proxy;

private:
  nux::CairoGraphics _util_cg;
  int _padding;
  
  sigc::connection _on_indicator_activate_changed_connection;
  sigc::connection _on_indicator_updated_connection;
  sigc::connection _on_panelstyle_changed_connection;

  gulong _on_font_changed_connection;
  static void OnFontChanged (GObject *gobject, GParamSpec *pspec, gpointer data);
};

#endif // PANEL_INDICATOR_OBJECT_ENTRY_VIEW_H
