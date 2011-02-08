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
#include <map>

#include "IndicatorObjectProxy.h"
#include "Introspectable.h"
#include "PanelIndicatorObjectView.h"
#include "StaticCairoText.h"
#include "WindowButtons.h"
#include "PanelTitlebarGrabAreaView.h"
#include "PluginAdapter.h"

#include <libbamf/libbamf.h>

class PanelMenuView : public PanelIndicatorObjectView
{
public:
  // This contains all the menubar logic for the Panel. Mainly it contains
  // the following states:
  // 1. Unmaximized window + no mouse hover
  // 2. Unmaximized window + mouse hover
  // 3. Unmaximized window + active menu (Alt+F/arrow key nav)
  // 4. Maximized window + no mouse hover
  // 5. Maximized window + mouse hover
  // 6. Maximized window + active menu
  //
  // It also deals with undecorating maximized windows (and redecorating them
  // on unmaximize)

  PanelMenuView (int padding = 6);
  ~PanelMenuView ();

  void FullRedraw ();

  virtual long ProcessEvent (nux::IEvent &ievent, long TraverseInfo, long ProcessEventInfo);
  virtual void Draw (nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent (nux::GraphicsEngine &GfxContext, bool force_draw);
  virtual long PostLayoutManagement (long LayoutResult);
  
  void SetProxy (IndicatorObjectProxy *proxy);
 
  void OnEntryAdded (IndicatorObjectEntryProxy *proxy);
  void OnEntryMoved (IndicatorObjectEntryProxy *proxy);
  void OnEntryRemoved (IndicatorObjectEntryProxy *proxy);
  void OnEntryRefreshed (PanelIndicatorObjectEntryView *view);
  void OnActiveChanged (PanelIndicatorObjectEntryView *view, bool is_active);
  void OnActiveWindowChanged (BamfView *old_view, BamfView *new_view);
  void OnNameChanged (gchar* new_name, gchar* old_name);

  void OnSpreadInitiate (std::list <guint32> &);
  void OnSpreadTerminate (std::list <guint32> &);
  void OnWindowMinimized (guint32 xid);
  void OnWindowUnminimized (guint32 xid);
  void OnWindowUnmapped (guint32 xid);
  void OnWindowMaximized (guint32 xid);
  void OnWindowRestored  (guint32 xid);

  void OnMaximizedGrab (int x, int y);
  void OnMouseMiddleClicked ();

  void Refresh ();
  void AllMenusClosed ();
  
  void OnCloseClicked ();
  void OnMinimizeClicked ();
  void OnRestoreClicked ();
  void OnWindowButtonsRedraw ();

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
  nux::IntrusiveSP<nux::IOpenGLBaseTexture> _gradient_texture;
  nux::BaseTexture        *_title_tex;

  bool _is_inside;
  bool _is_grabbed;
  bool _is_maximized; 
  PanelIndicatorObjectEntryView *_last_active_view;

  WindowButtons * _window_buttons;
  PanelTitlebarGrabArea * _panel_titlebar_grab_area;

  std::map<guint32, bool> _decor_map;
  int _padding;
  gpointer _name_changed_callback_instance;
  gulong _name_changed_callback_id;
};
#endif
