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
#include <memory>

#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxImage/CairoGraphics.h>
#include <gdk/gdkx.h>

#include <UnityCore/DBusIndicators.h>

#include "BackgroundEffectHelper.h"
#include "Introspectable.h"
#include "PanelMenuView.h"
#include "PanelTray.h"
#include "PanelIndicatorsView.h"

namespace unity
{

class PanelView : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PanelView, nux::View);
public:
  PanelView(NUX_FILE_LINE_PROTO);
  ~PanelView();

  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  void PreLayoutManagement();
  long PostLayoutManagement(long LayoutResult);

  void OnObjectAdded(indicator::Indicator::Ptr const& proxy);
  void OnObjectRemoved(indicator::Indicator::Ptr const& proxy);
  void OnIndicatorViewUpdated(PanelIndicatorEntryView* view);
  void OnMenuPointerMoved(int x, int y);
  void OnEntryActivateRequest(std::string const& entry_id);
  void OnEntryActivated(std::string const& entry_id, nux::Rect const& geo);
  void OnSynced();
  void OnEntryShowMenu(std::string const& entry_id, unsigned int xid, int x, int y,
                       unsigned int button, unsigned int timestamp);

  void SetPrimary(bool primary);
  bool GetPrimary();
  void SetMonitor(int monitor);

  bool FirstMenuShow();

  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);
  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  void TrackMenuPointer();

  unsigned int GetTrayXid ();

protected:
  // Introspectable methods
  std::string GetName() const;
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
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
  indicator::DBusIndicators::Ptr _remote;
  // No ownership is taken for these views, that is done by the AddChild method.

  PanelMenuView*           _menu_view;
  PanelTray*               _tray;
  PanelIndicatorsView*     _indicators;

  std::unique_ptr<nux::AbstractPaintLayer> bg_layer_;
  std::unique_ptr<nux::ColorLayer> bg_darken_layer_;
  BaseTexturePtr           _panel_sheen;
  nux::HLayout*            _layout;

  int _last_width;
  int _last_height;

  nux::Color  _bg_color;
  bool        _is_dirty;
  float       _opacity;
  bool        _opacity_maximized_toggle;
  bool        _needs_geo_sync;
  bool        _is_primary;
  int         _monitor;

  bool        _overlay_is_open;
  std::string _active_overlay;
  guint       _handle_dash_hidden;
  guint       _handle_dash_shown;
  guint       _handle_bg_color_update;
  guint       _track_menu_pointer_id;
  nux::Point  _tracked_pointer_pos;

  std::vector<sigc::connection> _on_indicator_updated_connections;
  std::vector<sigc::connection> _maximized_opacity_toggle_connections;
  BackgroundEffectHelper bg_effect_helper_;
  nux::ObjectPtr <nux::IOpenGLBaseTexture> bg_blur_texture_;
};

}

#endif // PANEL_VIEW_H
