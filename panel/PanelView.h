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
#include <NuxGraphics/CairoGraphics.h>
#include <gdk/gdkx.h>

#include <UnityCore/DBusIndicators.h>

#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/Introspectable.h"
#include "PanelMenuView.h"
#include "PanelTray.h"
#include "PanelIndicatorsView.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{

class PanelView : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PanelView, nux::View);
public:
  PanelView(NUX_FILE_LINE_PROTO);
  ~PanelView();

  void SetPrimary(bool primary);
  bool GetPrimary() const;

  void SetMonitor(int monitor);
  int GetMonitor() const;

  bool IsActive() const;
  bool FirstMenuShow() const;

  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);
  void SetMenuShowTimings(int fadein, int fadeout, int discovery,
                          int discovery_fadein, int discovery_fadeout);

  Window GetTrayXid() const;

  void SetLauncherWidth(int width);

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  void OnObjectAdded(indicator::Indicator::Ptr const& proxy);
  void OnObjectRemoved(indicator::Indicator::Ptr const& proxy);
  void OnIndicatorViewUpdated(PanelIndicatorEntryView* view);
  void OnMenuPointerMoved(int x, int y);
  void OnEntryActivateRequest(std::string const& entry_id);
  void OnEntryActivated(std::string const& entry_id, nux::Rect const& geo);
  void OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button);

private:
  void OnBackgroundUpdate(GVariant *data);
  void OnOverlayShown(GVariant *data);
  void OnOverlayHidden(GVariant *data);

  void UpdateBackground();
  void ForceUpdateBackground();
  bool TrackMenuPointer();
  void SyncGeometries();
  void AddPanelView(PanelIndicatorsView* child, unsigned int stretchFactor);

  indicator::DBusIndicators::Ptr remote_;

  // No ownership is taken for these views, that is done by the AddChild method.
  PanelMenuView* menu_view_;
  PanelTray* tray_;
  PanelIndicatorsView* indicators_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> PaintLayerPtr;
  PaintLayerPtr bg_layer_;
  PaintLayerPtr bg_darken_layer_;
  nux::ObjectPtr<nux::BaseTexture> panel_sheen_;
  nux::HLayout* layout_;

  nux::ObjectPtr<nux::BaseTexture> bg_refine_tex_;
  nux::ObjectPtr<nux::BaseTexture> bg_refine_no_refine_tex_;
  std::unique_ptr<nux::AbstractPaintLayer> bg_refine_layer_;
  nux::ObjectPtr<nux::BaseTexture> bg_refine_single_column_tex_;
  std::unique_ptr<nux::AbstractPaintLayer> bg_refine_single_column_layer_;

  nux::Geometry last_geo_;
  nux::Color bg_color_;
  std::string active_overlay_;
  nux::Point  tracked_pointer_pos_;

  bool is_dirty_;
  bool opacity_maximized_toggle_;
  bool needs_geo_sync_;
  bool is_primary_;
  bool overlay_is_open_;
  float opacity_;
  int monitor_;
  int stored_dash_width_;
  int launcher_width_;
  bool refine_is_open_;

  std::vector<sigc::connection> on_indicator_updated_connections_;
  std::vector<sigc::connection> maximized_opacity_toggle_connections_;
  BackgroundEffectHelper bg_effect_helper_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> bg_blur_texture_;
  UBusManager ubus_manager_;
  glib::Source::UniquePtr track_menu_pointer_timeout_;
};

}

#endif // PANEL_VIEW_H
