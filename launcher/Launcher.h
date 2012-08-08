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
 * Authored by: Jason Smith <jason.smith@canonical.com>
 * Authored by: Jay Taoko <jay.taoko@canonical.com>
 */

#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <Nux/View.h>
#include <Nux/BaseWindow.h>
#include <Nux/TimerProc.h>
#include <NuxGraphics/IOpenGLAsmShader.h>

#include "PointerBarrier.h"
#include "unity-shared/AbstractIconRenderer.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "DNDCollectionWindow.h"
#include "DndData.h"
#include "EdgeBarrierController.h"
#include "unity-shared/Introspectable.h"
#include "LauncherModel.h"
#include "LauncherOptions.h"
#include "LauncherDragWindow.h"
#include "LauncherHideMachine.h"
#include "LauncherHoverMachine.h"
#include "unity-shared/UBusWrapper.h"
#include "SoftwareCenterLauncherIcon.h"


namespace unity
{
namespace launcher
{
extern const char window_title[];

class AbstractLauncherIcon;

class Launcher : public unity::debug::Introspectable, public nux::View, public ui::EdgeBarrierSubscriber
{
  NUX_DECLARE_OBJECT_TYPE(Launcher, nux::View);
public:

  Launcher(nux::BaseWindow* parent, nux::ObjectPtr<DNDCollectionWindow> const& collection_window, NUX_FILE_LINE_PROTO);

  nux::Property<Display*> display;
  nux::Property<int> monitor;
  nux::Property<Options::Ptr> options;

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  AbstractLauncherIcon::Ptr GetSelectedMenuIcon() const;

  void SetIconSize(int tile_size, int icon_size);
  int GetIconSize() const;

  bool Hidden() const
  {
    return _hidden;
  }

  void ForceReveal(bool force);
  void ShowShortcuts(bool show);

  void SetModel(LauncherModel::Ptr model);
  LauncherModel::Ptr GetModel() const;

  void StartKeyShowLauncher();
  void EndKeyShowLauncher();

  void EnsureIconOnScreen(AbstractLauncherIcon::Ptr icon);

  void SetBacklightMode(BacklightMode mode);
  BacklightMode GetBacklightMode() const;
  bool IsBackLightModeToggles() const;

  nux::BaseWindow* GetParent() const
  {
    return _parent;
  };

  nux::ObjectPtr<nux::View> GetActiveTooltip() const;  // nullptr = no tooltip

  virtual void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags);

  virtual void RecvQuicklistOpened(QuicklistView* quicklist);
  virtual void RecvQuicklistClosed(QuicklistView* quicklist);

  int GetMouseX() const;
  int GetMouseY() const;

  void Resize();

  int GetDragDelta() const;
  void SetHover(bool hovered);

  sigc::signal<void, char*, AbstractLauncherIcon::Ptr> launcher_addrequest;
  sigc::signal<void, AbstractLauncherIcon::Ptr> launcher_removerequest;
  sigc::signal<void, AbstractLauncherIcon::Ptr> icon_animation_complete;
  sigc::signal<void> selection_change;
  sigc::signal<void> hidden_changed;
  sigc::signal<void> sc_launcher_icon_animation;
  sigc::connection sc_launcher_icon_animation_connection;

  virtual bool InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character);

  void EnterKeyNavMode();
  void ExitKeyNavMode();
  bool IsInKeyNavMode() const;

  bool IsOverlayOpen() const;

  static const int ANIM_DURATION_SHORT;

  void RenderIconToTexture(nux::GraphicsEngine& GfxContext, AbstractLauncherIcon::Ptr icon, nux::ObjectPtr<nux::IOpenGLBaseTexture> texture);

  virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);
protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(GVariantBuilder* builder);

  void ProcessDndEnter();
  void ProcessDndLeave();
  void ProcessDndMove(int x, int y, std::list<char*> mimes);
  void ProcessDndDrop(int x, int y);
private:
  typedef nux::ObjectPtr<nux::BaseTexture> BaseTexturePtr;

  LauncherHideMode GetHideMode() const;
  void SetHideMode(LauncherHideMode hidemode);

  typedef enum
  {
    ACTION_NONE,
    ACTION_DRAG_LAUNCHER,
    ACTION_DRAG_ICON,
    ACTION_DRAG_EXTERNAL,
  } LauncherActionState;

  typedef enum
  {
    TIME_ENTER,
    TIME_LEAVE,
    TIME_DRAG_END,
    TIME_DRAG_THRESHOLD,
    TIME_AUTOHIDE,
    TIME_DRAG_EDGE_TOUCH,
    TIME_DRAG_OUT,
    TIME_TAP_SUPER,
    TIME_SUPER_PRESSED,

    TIME_LAST
  } LauncherActionTimes;

  void ConfigureBarrier();

  void OnOptionsChanged(Options::Ptr options);
  void OnOptionChanged();
  void UpdateOptions(Options::Ptr options);

  void OnWindowMapped(guint32 xid);
  void OnWindowUnmapped(guint32 xid);

  void OnDragStart(const nux::GestureEvent &event);
  void OnDragUpdate(const nux::GestureEvent &event);
  void OnDragFinish(const nux::GestureEvent &event);

  bool HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event);

  void OnPluginStateChanged();

  void OnSelectionChanged(AbstractLauncherIcon::Ptr selection);

  bool StrutHack();
  bool StartIconDragTimeout();
  bool OnScrollTimeout();
  bool OnUpdateDragManagerTimeout();

  void SetMousePosition(int x, int y);

  void SetStateMouseOverLauncher(bool over_launcher);

  bool MouseBeyondDragThreshold() const;

  void OnDragWindowAnimCompleted();

  bool IconNeedsAnimation(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  bool IconDrawEdgeOnly(AbstractLauncherIcon::Ptr icon) const;
  bool AnimationInProgress() const;

  void SetActionState(LauncherActionState actionstate);
  LauncherActionState GetActionState() const;

  void EnsureAnimation();
  void EnsureScrollTimer();

  bool MouseOverTopScrollArea();
  bool MouseOverTopScrollExtrema();

  bool MouseOverBottomScrollArea();
  bool MouseOverBottomScrollExtrema();

  float DnDStartProgress(struct timespec const& current) const;
  float DnDExitProgress(struct timespec const& current) const;
  float GetHoverProgress(struct timespec const& current) const;
  float AutohideProgress(struct timespec const& current) const;
  float DragThresholdProgress(struct timespec const& current) const;
  float DragHideProgress(struct timespec const& current) const;
  float DragOutProgress(struct timespec const& current) const;
  float IconDesatValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconPresentProgress(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconUrgentProgress(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconShimmerProgress(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconUrgentPulseValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconPulseOnceValue(AbstractLauncherIcon::Ptr icon, struct timespec const &current) const;
  float IconUrgentWiggleValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconStartingBlinkValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconStartingPulseValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconBackgroundIntensity(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconProgressBias(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconDropDimValue(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconCenterTransitionProgress(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;
  float IconVisibleProgress(AbstractLauncherIcon::Ptr icon, struct timespec const& current) const;

  void SetHidden(bool hidden);

  void  SetDndDelta(float x, float y, nux::Geometry const& geo, timespec const& current);
  float DragLimiter(float x);

  void SetupRenderArg(AbstractLauncherIcon::Ptr icon, struct timespec const& current, ui::RenderArg& arg);
  void FillRenderArg(AbstractLauncherIcon::Ptr icon,
                     ui::RenderArg& arg,
                     nux::Point3& center,
                     nux::Geometry const& parent_abs_geo,
                     float folding_threshold,
                     float folded_size,
                     float folded_spacing,
                     float autohide_offset,
                     float folded_z_distance,
                     float animation_neg_rads,
                     struct timespec const& current);

  void RenderArgs(std::list<ui::RenderArg> &launcher_args,
                  nux::Geometry& box_geo, float* launcher_alpha, nux::Geometry const& parent_abs_geo);

  void OnIconAdded(AbstractLauncherIcon::Ptr icon);
  void OnIconRemoved(AbstractLauncherIcon::Ptr icon);
  void OnOrderChanged();

  void OnIconNeedsRedraw(AbstractLauncherIcon::Ptr icon);
  void OnTooltipVisible(nux::ObjectPtr<nux::View> view);

  void OnOverlayHidden(GVariant* data);
  void OnOverlayShown(GVariant* data);

  void DesaturateIcons();
  void SaturateIcons();

  void OnBGColorChanged(GVariant *data);

  void OnLockHideChanged(GVariant *data);

  void OnActionDone(GVariant* data);

  AbstractLauncherIcon::Ptr MouseIconIntersection(int x, int y);
  void EventLogic();
  void MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void StartIconDragRequest(int x, int y);
  void StartIconDrag(AbstractLauncherIcon::Ptr icon);
  void EndIconDrag();
  void UpdateDragWindowPosition(int x, int y);

  void ResetMouseDragState();

  float GetAutohidePositionMin() const;
  float GetAutohidePositionMax() const;

  virtual long PostLayoutManagement(long LayoutResult);

  void SetOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture);
  void RestoreSystemRenderTarget();

  void OnDisplayChanged(Display* display);
  void OnDNDDataCollected(const std::list<char*>& mimes);

  void DndReset();
  void DndHoveredIconReset();
  void DndTimeoutSetup();

  LauncherModel::Ptr _model;
  nux::BaseWindow* _parent;
  nux::ObjectPtr<nux::View> _active_tooltip;
  QuicklistView* _active_quicklist;

  nux::HLayout* m_Layout;

  // used by keyboard/a11y-navigation
  AbstractLauncherIcon::Ptr _icon_under_mouse;
  AbstractLauncherIcon::Ptr _icon_mouse_down;
  AbstractLauncherIcon::Ptr _drag_icon;
  AbstractLauncherIcon::Ptr _dnd_hovered_icon;

  bool _hovered;
  bool _hidden;
  bool _scroll_limit_reached;
  bool _render_drag_window;
  bool _shortcuts_shown;
  bool _data_checked;
  bool _steal_drag;
  bool _drag_edge_touching;
  bool _initial_drag_animation;
  bool _dash_is_open;
  bool _hud_is_open;

  BacklightMode _backlight_mode;

  float _folded_angle;
  float _neg_folded_angle;
  float _folded_z_distance;
  float _last_delta_y;
  float _edge_overcome_pressure;

  LauncherActionState _launcher_action_state;
  LaunchAnimation _launch_animation;
  UrgentAnimation _urgent_animation;

  int _space_between_icons;
  int _icon_image_size;
  int _icon_image_size_delta;
  int _icon_glow_size;
  int _icon_size;
  int _dnd_delta_y;
  int _dnd_delta_x;
  int _postreveal_mousemove_delta_x;
  int _postreveal_mousemove_delta_y;
  int _launcher_drag_delta;
  int _launcher_drag_delta_max;
  int _launcher_drag_delta_min;
  int _enter_y;
  int _last_button_press;
  float _drag_out_delta_x;
  bool _drag_gesture_ongoing;
  float _last_reveal_progress;

  nux::Point2 _mouse_position;
  nux::ObjectPtr<nux::IOpenGLBaseTexture> _offscreen_drag_texture;
  nux::ObjectPtr<LauncherDragWindow> _drag_window;
  nux::ObjectPtr<unity::DNDCollectionWindow> _collection_window;
  LauncherHideMachine _hide_machine;
  LauncherHoverMachine _hover_machine;

  unity::DndData _dnd_data;
  nux::DndAction _drag_action;
  Atom _selection_atom;

  struct timespec  _times[TIME_LAST];

  nux::Color _background_color;
  BaseTexturePtr launcher_sheen_;
  BaseTexturePtr launcher_pressure_effect_;

  ui::AbstractIconRenderer::Ptr icon_renderer;
  BackgroundEffectHelper bg_effect_helper_;

  UBusManager ubus_;
  glib::SourceManager sources_;
};

}
}

#endif // LAUNCHER_H
