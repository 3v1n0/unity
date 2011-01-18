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

#ifndef PANEL_VIEW_H
#define PANEL_VIEW_H

#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>

#include "PanelHomeButton.h"
#include "PanelMenuView.h"
#include "IndicatorObjectFactoryRemote.h"
#include "Introspectable.h"

class PanelView : public Introspectable, public nux::View
{
public:
  PanelView (NUX_FILE_LINE_PROTO);
  ~PanelView ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);

  virtual void PreLayoutManagement ();
  virtual long PostLayoutManagement (long LayoutResult);
  
  void OnObjectAdded (IndicatorObjectProxy *proxy);
  void OnMenuPointerMoved (int x, int y);
  void OnEntryActivateRequest (const char *entry_id);
  void OnEntryActivated (const char *entry_id);
  
  PanelHomeButton * HomeButton ();
  std::list<nux::Object *> *GetChildren ();

protected:
  // Introspectable methods
  void AddChild (Introspectable *child);
  void RemoveChild (Introspectable *child);
  const gchar * GetName ();
  const gchar * GetChildsName (); 
  void AddProperties (GVariantBuilder *builder);

private:
  void UpdateBackground ();

private:
  IndicatorObjectFactoryRemote *_remote;

  PanelHomeButton          *_home_button;
  PanelMenuView            *_menu_view;
  nux::AbstractPaintLayer  *_bg_layer;
  nux::HLayout             *_layout;
  std::list<nux::Object *> *_children;

  int _last_width;
  int _last_height;
};

#endif // PANEL_VIEW_H
