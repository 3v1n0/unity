// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd
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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 */

#ifndef SWITCHERVIEW_H
#define SWITCHERVIEW_H

#include "DeltaTracker.h"
#include "SwitcherModel.h"
#include "unity-shared/AbstractIconRenderer.h"
#include "unity-shared/StaticCairoText.h"
#include "unity-shared/LayoutSystem.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/Introspectable.h"
#include "unity-shared/UnityWindowView.h"

#include <Nux/View.h>
#include <NuxCore/Property.h>

#include <UnityCore/GLibSource.h>


namespace unity
{
namespace launcher
{
class AbstractLauncherIcon;
}
namespace switcher
{

class SwitcherView : public ui::UnityWindowView
{
  NUX_DECLARE_OBJECT_TYPE(SwitcherView, ui::UnityWindowView);
public:
  typedef nux::ObjectPtr<SwitcherView> Ptr;

  SwitcherView(nux::BaseWindow *parent);

  ui::LayoutWindow::Vector ExternalTargets();

  void SetModel(SwitcherModel::Ptr model);
  SwitcherModel::Ptr GetModel();

  nux::Property<bool> render_boxes;
  nux::Property<bool> animate;
  nux::Property<int> border_size;
  nux::Property<int> flat_spacing;
  nux::Property<int> icon_size;
  nux::Property<int> minimum_spacing;
  nux::Property<int> tile_size;
  nux::Property<int> vertical_size;
  nux::Property<int> text_size;
  nux::Property<int> animation_length;
  nux::Property<int> monitor;
  nux::Property<double> spread_size;

  // Returns the index of the icon at the given position, in window coordinates.
  // If there's no icon there, -1 is returned.
  int IconIndexAt(int x, int y) const;
  int DetailIconIdexAt(int x, int y) const;

  /* void; int icon_index, int button*/
  sigc::signal<void, int, int> switcher_mouse_down;
  sigc::signal<void, int, int> switcher_mouse_up;

  /* void; int icon_index */
  sigc::signal<void, int> switcher_mouse_move;

  /* void; */
  sigc::signal<void> switcher_next;
  sigc::signal<void> switcher_prev;
  sigc::signal<void> switcher_start_detail;
  sigc::signal<void> switcher_stop_detail;

  /* void; bool visible */
  sigc::signal<void, bool> hide_request;

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);
  IntrospectableList GetIntrospectableChildren();

  void PreDraw(nux::GraphicsEngine& GfxContext, bool force_draw);
  void DrawOverlay(nux::GraphicsEngine& GfxContext, bool force_draw, nux::Geometry const& clip);
  nux::Geometry GetBackgroundGeometry();

  ui::RenderArg InterpolateRenderArgs(ui::RenderArg const& start, ui::RenderArg const& end, float progress);
  nux::Geometry InterpolateBackground(nux::Geometry const& start, nux::Geometry const& end, float progress);

  std::list<ui::RenderArg> RenderArgsFlat(nux::Geometry& background_geo, int selection, float progress);

  ui::RenderArg CreateBaseArgForIcon(launcher::AbstractLauncherIcon::Ptr const& icon);

  virtual bool InspectKeyEvent(unsigned int eventType, unsigned int keysym, const char* character);
  virtual nux::Area* FindKeyFocusArea(unsigned int key_symbol, unsigned long x11_key_code, unsigned long special_keys_state);

private:
  void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  void HandleDetailMouseMove(int x, int y);
  void HandleMouseMove(int x, int y);

  void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void HandleDetailMouseDown(int x, int y, int button);
  void HandleMouseDown(int x, int y, int button);

  void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void HandleDetailMouseUp(int x, int y, int button);
  void HandleMouseUp(int x, int y, int button);

  void RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags);
  void HandleDetailMouseWheel(int wheel_delta);
  void HandleMouseWheel(int wheel_delta);

  void OnSelectionChanged(launcher::AbstractLauncherIcon::Ptr const& selection);
  void OnDetailSelectionChanged (bool detail);
  void OnDetailSelectionIndexChanged (unsigned int index);

  void OnIconSizeChanged (int size);
  void OnTileSizeChanged (int size);

  nux::Geometry UpdateRenderTargets(float progress);
  void ResizeRenderTargets(nux::Geometry const& layout_geo, float progress);
  void OffsetRenderTargets(int x, int y);

  nux::Size SpreadSize();

  double GetCurrentProgress();

  void SaveTime();
  void ResetTimer();
  void SaveLast();

  bool CheckMouseInsideBackground(int x, int y) const;
  void MouseHandlingBackToNormal();

  SwitcherModel::Ptr model_;
  ui::LayoutSystem layout_system_;
  ui::AbstractIconRenderer::Ptr icon_renderer_;
  nux::ObjectPtr<StaticCairoText> text_view_;

  int last_icon_selected_;
  int last_detail_icon_selected_;
  bool target_sizes_set_;
  bool check_mouse_first_time_;

  DeltaTracker delta_tracker_;

  std::list<ui::RenderArg> last_args_;
  std::list<ui::RenderArg> saved_args_;

  nux::Geometry last_background_;
  nux::Geometry saved_background_;

  ui::LayoutWindow::Vector render_targets_;

  timespec current_;
  timespec save_time_;

  glib::Source::UniquePtr redraw_idle_;

  friend class TestSwitcherView;
};

}
}

#endif // SWITCHERVIEW_H

