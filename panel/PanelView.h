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

#include <Nux/Nux.h>
#include <Nux/View.h>
#include <Nux/TextureArea.h>
#include <NuxGraphics/GraphicsEngine.h>
#include <NuxGraphics/CairoGraphics.h>
#include <gdk/gdkx.h>

#include "launcher/EdgeBarrierController.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/MenuManager.h"
#include "unity-shared/MockableBaseWindow.h"
#include "PanelMenuView.h"
#include "PanelTray.h"
#include "PanelIndicatorsView.h"
#include "unity-shared/UBusWrapper.h"

namespace unity
{
namespace panel
{

class PanelView : public unity::debug::Introspectable,
                  public ui::EdgeBarrierSubscriber,
                  public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(PanelView, nux::View);
public:
  PanelView(MockableBaseWindow* parent, menu::Manager::Ptr const&, NUX_FILE_LINE_PROTO);
  ~PanelView();

  MockableBaseWindow* GetParent() const
  {
    return parent_;
  };

  void SetMonitor(int monitor);
  int GetMonitor() const;

  bool IsActive() const;
  bool InOverlayMode() const;

  void SetOpacity(float opacity);
  void SetOpacityMaximizedToggle(bool enabled);

  Window GetTrayXid() const;
  int GetStoredDashWidth() const;

  void SetLauncherWidth(int width);

  bool IsMouseInsideIndicator(nux::Point const& mouse_position) const;

  ui::EdgeBarrierSubscriber::Result HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event) override;

protected:
  void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  void PreLayoutManagement();

  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

  void OnObjectAdded(indicator::Indicator::Ptr const& proxy);
  void OnObjectRemoved(indicator::Indicator::Ptr const& proxy);
  void OnIndicatorViewUpdated();
  void OnMenuPointerMoved(int x, int y);
  void OnEntryActivated(std::string const& entry_id, nux::Rect const& geo);
  void OnEntryShowMenu(std::string const& entry_id, unsigned xid, int x, int y, unsigned button);

private:
  void OnBackgroundUpdate(nux::Color const&);
  void OnOverlayShown(GVariant *data);
  void OnOverlayHidden(GVariant *data);
  void OnSpreadInitiate();
  void OnSpreadTerminate();
  void EnableOverlayMode(bool);

  bool ActivateFirstSensitive();
  bool ActivateEntry(std::string const& entry_id);
  void Resize(nux::Point const& offset, int width);
  bool IsTransparent();
  void UpdateBackground();
  void ForceUpdateBackground();
  bool TrackMenuPointer();
  void SyncGeometries();
  void AddPanelView(PanelIndicatorsView* child, unsigned int stretchFactor);
  
  void OnDPIChanged();

  MockableBaseWindow* parent_;
  indicator::Indicators::Ptr remote_;

  // No ownership is taken for these views, that is done by the AddChild method.
  PanelMenuView* menu_view_;
  PanelTray* tray_;
  PanelIndicatorsView* indicators_;
  nux::HLayout* layout_;

  typedef std::unique_ptr<nux::AbstractPaintLayer> PaintLayerPtr;
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;
  PaintLayerPtr bg_layer_;
  PaintLayerPtr bg_darken_layer_;
  BaseTexturePtr panel_sheen_;

  BaseTexturePtr bg_refine_tex_;
  std::unique_ptr<nux::AbstractPaintLayer> bg_refine_layer_;
  BaseTexturePtr bg_refine_single_column_tex_;
  std::unique_ptr<nux::AbstractPaintLayer> bg_refine_single_column_layer_;

  std::string active_overlay_;
  nux::Point  tracked_pointer_pos_;

  bool is_dirty_;
  bool opacity_maximized_toggle_;
  bool needs_geo_sync_;
  bool overlay_is_open_;
  float opacity_;
  int monitor_;
  int stored_dash_width_;
  int launcher_width_;

  connection::Manager on_indicator_updated_connections_;
  connection::Manager maximized_opacity_toggle_connections_;
  BackgroundEffectHelper bg_effect_helper_;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> bg_blur_texture_;
  UBusManager ubus_manager_;
  glib::Source::UniquePtr track_menu_pointer_timeout_;
};

} // namespace panel
} // namespace unity

#endif // PANEL_VIEW_H
