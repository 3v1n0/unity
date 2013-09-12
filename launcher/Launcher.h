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
#include <NuxCore/Animation.h>
#include <NuxGraphics/GestureEvent.h>
#ifndef USE_GLES
# include <NuxGraphics/IOpenGLAsmShader.h>
#endif

#include "unity-shared/AbstractIconRenderer.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "DevicesSettings.h"
#include "DndData.h"
#include "unity-shared/Introspectable.h"
#include "LauncherModel.h"
#include "LauncherOptions.h"
#include "LauncherDragWindow.h"
#include "LauncherHideMachine.h"
#include "LauncherHoverMachine.h"
#include "unity-shared/MockableBaseWindow.h"
#include "unity-shared/UBusWrapper.h"
#include "SoftwareCenterLauncherIcon.h"
#include "TooltipManager.h"

#ifdef USE_X11
# include "PointerBarrier.h"
# include "EdgeBarrierController.h"
#endif

namespace unity
{
namespace launcher
{
extern const char* window_title;

class AbstractLauncherIcon;

class Launcher : public unity::debug::Introspectable,
#ifdef USE_X11
                 // TODO: abstract this into a more generic class.
                 public ui::EdgeBarrierSubscriber,
#endif
                 public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(Launcher, nux::View);
public:

  Launcher(MockableBaseWindow* parent, NUX_FILE_LINE_PROTO);

  nux::Property<int> monitor;
  nux::Property<Options::Ptr> options;

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);

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

  void EnsureIconOnScreen(AbstractLauncherIcon::Ptr const& icon);

  void SetBacklightMode(BacklightMode mode);
  BacklightMode GetBacklightMode() const;
  bool IsBackLightModeToggles() const;

  MockableBaseWindow* GetParent() const
  {
    return _parent;
  };

  nux::ObjectPtr<nux::View> const& GetActiveTooltip() const;
  LauncherDragWindow::Ptr const& GetDraggedIcon() const;

  virtual void RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags);
  virtual void RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags);

  virtual void RecvQuicklistOpened(nux::ObjectPtr<QuicklistView> const& quicklist);
  virtual void RecvQuicklistClosed(nux::ObjectPtr<QuicklistView> const& quicklist);

  void ScrollLauncher(int wheel_delta);

  int GetMouseX() const;
  int GetMouseY() const;

  void Resize(nux::Point const& offset, int height);

  int GetDragDelta() const;
  void SetHover(bool hovered);

  void DndStarted(std::string const& mimes);
  void DndFinished();
  void SetDndQuirk();
  void UnsetDndQuirk();

  sigc::signal<void, std::string const&, AbstractLauncherIcon::Ptr const&> add_request;
  sigc::signal<void, AbstractLauncherIcon::Ptr const&> remove_request;
  sigc::signal<void> selection_change;
  sigc::signal<void> hidden_changed;
  sigc::signal<void> sc_launcher_icon_animation;

  virtual bool InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character);

  void EnterKeyNavMode();
  void ExitKeyNavMode();
  bool IsInKeyNavMode() const;

  bool IsOverlayOpen() const;

  static const int ANIM_DURATION_SHORT;

  void RenderIconToTexture(nux::GraphicsEngine&, nux::ObjectPtr<nux::IOpenGLBaseTexture> const&, AbstractLauncherIcon::Ptr const&);

#ifdef NUX_GESTURES_SUPPORT
  virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);
#endif

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

  void ConfigureBarrier();

  void OnMonitorChanged(int monitor);
  void OnOptionsChanged(Options::Ptr options);
  void OnOptionChanged();
  void UpdateOptions(Options::Ptr options);

#ifdef NUX_GESTURES_SUPPORT
  void OnDragStart(const nux::GestureEvent &event);
  void OnDragUpdate(const nux::GestureEvent &event);
  void OnDragFinish(const nux::GestureEvent &event);
#endif

#ifdef USE_X11
  ui::EdgeBarrierSubscriber::Result HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event);
#endif

  void OnPluginStateChanged();

  void OnSelectionChanged(AbstractLauncherIcon::Ptr const& selection);

  bool StrutHack();
  bool StartIconDragTimeout(int x, int y);
  bool OnScrollTimeout();

  void SetUrgentTimer(int urgent_wiggle_period);
  void WiggleUrgentIcon(AbstractLauncherIcon::Ptr const& icon);
  void HandleUrgentIcon(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current);
  bool OnUrgentTimeout();

  void SetMousePosition(int x, int y);
  void SetIconUnderMouse(AbstractLauncherIcon::Ptr const& icon);

  void SetStateMouseOverLauncher(bool over_launcher);

  bool MouseBeyondDragThreshold() const;

  void OnDragWindowAnimCompleted();

  bool IconNeedsAnimation(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  bool IconDrawEdgeOnly(AbstractLauncherIcon::Ptr const& icon) const;
  bool AnimationInProgress() const;

  void SetActionState(LauncherActionState actionstate);
  LauncherActionState GetActionState() const;

  void EnsureScrollTimer();
  bool MouseOverTopScrollArea();
  bool MouseOverBottomScrollArea();

  float DragOutProgress() const;
  float IconDesatValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconPresentProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconUnfoldProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconUrgentProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconShimmerProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconUrgentPulseValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconPulseOnceValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const &current) const;
  float IconUrgentWiggleValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconStartingBlinkValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconStartingPulseValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconBackgroundIntensity(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconProgressBias(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconDropDimValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconCenterTransitionProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;
  float IconVisibleProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const;

  void SetHidden(bool hidden);

  void UpdateChangeInMousePosition(int delta_x, int delta_y);

  void  SetDndDelta(float x, float y, nux::Geometry const& geo, timespec const& current);
  float DragLimiter(float x);

  void SetupRenderArg(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current, ui::RenderArg& arg);
  void FillRenderArg(AbstractLauncherIcon::Ptr const& icon,
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

  void OnIconAdded(AbstractLauncherIcon::Ptr const& icon);
  void OnIconRemoved(AbstractLauncherIcon::Ptr const& icon);

  void OnTooltipVisible(nux::ObjectPtr<nux::View> view);

  void OnOverlayHidden(GVariant* data);
  void OnOverlayShown(GVariant* data);

  void DesaturateIcons();
  void SaturateIcons();

  void OnBGColorChanged(GVariant *data);

  void OnLockHideChanged(GVariant *data);

  void OnActionDone(GVariant* data);

  virtual AbstractLauncherIcon::Ptr MouseIconIntersection(int x, int y) const;
  void EventLogic();
  void MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void StartIconDragRequest(int x, int y);
  void StartIconDrag(AbstractLauncherIcon::Ptr const& icon);
  void EndIconDrag();
  void ShowDragWindow();
  void UpdateDragWindowPosition(int x, int y);
  void HideDragWindow();

  void ResetMouseDragState();

  float GetAutohidePositionMin() const;
  float GetAutohidePositionMax() const;

  virtual long PostLayoutManagement(long LayoutResult);

  void DndReset();
  void DndHoveredIconReset();
  bool DndIsSpecialRequest(std::string const& uri) const;

  LauncherModel::Ptr _model;
  MockableBaseWindow* _parent;
  nux::ObjectPtr<nux::View> _active_tooltip;
  QuicklistView* _active_quicklist;

  // used by keyboard/a11y-navigation
  AbstractLauncherIcon::Ptr _icon_under_mouse;
  AbstractLauncherIcon::Ptr _icon_mouse_down;
  AbstractLauncherIcon::Ptr _drag_icon;
  AbstractLauncherIcon::Ptr _dnd_hovered_icon;

  bool _hovered;
  bool _hidden;
  bool _render_drag_window;
  bool _shortcuts_shown;
  bool _data_checked;
  bool _steal_drag;
  bool _drag_edge_touching;
  bool _initial_drag_animation;
  bool _dash_is_open;
  bool _hud_is_open;
  bool _folded;

  LauncherActionState _launcher_action_state;

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
  int _drag_icon_position;
  int _urgent_wiggle_time;
  bool _urgent_acked;
  bool _urgent_timer_running;
  bool _urgent_ack_needed;
  float _drag_out_delta_x;
  bool _drag_gesture_ongoing;
  float _last_reveal_progress;

  nux::Point _mouse_position;
  LauncherDragWindow::Ptr _drag_window;
  LauncherHideMachine _hide_machine;
  LauncherHoverMachine _hover_machine;
  TooltipManager tooltip_manager_;

  unity::DndData _dnd_data;
  nux::DndAction _drag_action;
  Atom _selection_atom;
  struct timespec _urgent_finished_time;

  BaseTexturePtr launcher_sheen_;
  BaseTexturePtr launcher_pressure_effect_;

  nux::animation::AnimateValue<float> auto_hide_animation_;
  nux::animation::AnimateValue<float> hover_animation_;
  nux::animation::AnimateValue<float> drag_over_animation_;
  nux::animation::AnimateValue<float> drag_out_animation_;
  nux::animation::AnimateValue<float> drag_icon_animation_;
  nux::animation::AnimateValue<float> dnd_hide_animation_;
  nux::animation::AnimateValue<float> dash_showing_animation_;

  ui::AbstractIconRenderer::Ptr icon_renderer;
  BackgroundEffectHelper bg_effect_helper_;

  UBusManager ubus_;
  glib::SourceManager sources_;

  friend class TestLauncher;
};

}
}

#endif // LAUNCHER_H
