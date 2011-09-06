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

#include <vector>

#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>

#include <gdk/gdkx.h>

#include <UnityCore/DBusIndicators.h>

#include "BackgroundEffectHelper.h"
#include "Introspectable.h"
#include "PanelMenuView.h"
#include "PanelTray.h"
#include "PanelIndicatorsView.h"
#include "PanelStyle.h"

namespace unity
{

class PanelView : public unity::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PanelView, nux::View);
public:
  PanelView(NUX_FILE_LINE_PROTO);
  ~PanelView();

  long ProcessEvent(nux::IEvent& ievent, long TraverseInfo, long ProcessEventInfo);
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void PreLayoutManagement();
  long PostLayoutManagement(long LayoutResult);

  void OnObjectAdded(indicator::Indicator::Ptr const& proxy);
  void OnObjectRemoved(indicator::Indicator::Ptr const& proxy);
  void OnIndicatorViewUpdated(PanelIndicatorEntryView* view);
  void OnMenuPointerMoved(int x, int y);
  void OnEntryActivateRequest(std::string const& entry_id);
  void OnEntryActivated(std::string const& entry_id);
  void OnSynced();
  void OnEntryShowMenu(std::string const& entry_id,
                       int x, int y, int timestamp, int button);

  void SetPrimary(bool primary);
  bool GetPrimary();
  void SetMonitor(int monitor);

  void StartFirstMenuShow();
  void EndFirstMenuShow();

  void SetOpacity(float opacity);

  unsigned int GetTrayXid ();

protected:
  // Introspectable methods
  const gchar* GetName();
  const gchar* GetChildsName();
  void AddProperties(GVariantBuilder* builder);

private:
  static void OnBackgroundUpdate  (GVariant *data, PanelView *self);
  static void OnDashShown         (GVariant *data, PanelView *self);
  static void OnDashHidden        (GVariant *data, PanelView *self);

  void UpdateBackground();
  void ForceUpdateBackground();
  void SyncGeometries();
  void AddPanelView(PanelIndicatorsView* child, unsigned int stretchFactor);

private:
  indicator::DBusIndicators::Ptr _remote;
  // No ownership is taken for these views, that is done by the AddChild method.

  PanelMenuView*           _menu_view;
  PanelTray*               _tray;
  PanelIndicatorsView*     _indicators;
  nux::AbstractPaintLayer* _bg_layer;
  nux::HLayout*            _layout;

  int _last_width;
  int _last_height;

  PanelStyle* _style;
  nux::Color  _bg_color;
  bool        _is_dirty;
  float       _opacity;
  bool        _needs_geo_sync;
  bool        _is_primary;
  int         _monitor;

  bool        _dash_is_open;
  guint       _handle_dash_hidden;
  guint       _handle_dash_shown;
  guint       _handle_bg_color_update;
  guint       _track_menu_pointer_id;

  std::vector<sigc::connection> _on_indicator_updated_connections;
  BackgroundEffectHelper bg_effect_helper_;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_blur_texture_;
};

}

#endif // PANEL_VIEW_H
