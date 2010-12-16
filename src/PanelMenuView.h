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

#ifndef PANEL_MENU_VIEW_H
#define PANEL_MENU_VIEW_H

#include <Nux/View.h>

#include "IndicatorObjectProxy.h"
#include "Introspectable.h"
#include "PanelIndicatorObjectView.h"
#include "StaticCairoText.h"
#include "WindowButtons.h"

#include <libbamf/libbamf.h>

class PanelMenuView : public PanelIndicatorObjectView
{
public:
  PanelMenuView ();
  ~PanelMenuView ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual long PostLayoutManagement (long LayoutResult);
  
  void SetProxy (IndicatorObjectProxy *proxy);
 
  void OnEntryAdded (IndicatorObjectEntryProxy *proxy);
  void OnEntryMoved (IndicatorObjectEntryProxy *proxy);
  void OnEntryRemoved (IndicatorObjectEntryProxy *proxy);
  void OnActiveChanged (PanelIndicatorObjectEntryView *view, bool is_active);
  void OnActiveWindowChanged (BamfView *old_view, BamfView *new_view);

  void Refresh ();
  void AllMenusClosed ();
  
  void OnCloseClicked ();
  void OnMinimizeClicked ();
  void OnRestoreClicked ();

protected:
  const gchar * GetName ();
  const gchar * GetChildsName ();
  void          AddProperties (GVariantBuilder *builder);

private:
  gchar * GetActiveViewName ();
  
private:
  BamfMatcher* _matcher;

  nux::AbstractPaintLayer *_title_layer;
  nux::HLayout            *_menu_layout;
  nux::CairoGraphics       _util_cg;

  bool _is_inside;
  bool _is_maximized; 
  PanelIndicatorObjectEntryView *_last_active_view;

  WindowButtons *_window_buttons;
};
#endif
