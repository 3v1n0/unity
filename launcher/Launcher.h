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
#include <Nux/TimerProc.h>
#include <NuxCore/Animation.h>
#include <NuxGraphics/GestureEvent.h>
#ifndef USE_GLES
# include <NuxGraphics/IOpenGLAsmShader.h>
#endif

#include "unity-shared/AbstractIconRenderer.h"
#include "unity-shared/BackgroundEffectHelper.h"
#include "unity-shared/EMConverter.h"
#include "unity-shared/RawPixel.h"
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
enum class LauncherPosition;

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

  void SetScrollInactiveIcons(bool scroll);
  void SetLauncherMinimizeWindow(bool click_to_minimize);

  void SetIconSize(int tile_size, int icon_size);
  int GetIconSize() const;

  bool Hidden() const
  {
    return hidden_;
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
    return parent_;
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
  sigc::signal<void> key_nav_terminate_request;

  virtual bool InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character);

  void EnterKeyNavMode();
  void ExitKeyNavMode();
  bool IsInKeyNavMode() const;
  bool IsOverlayOpen() const;

  void ClearTooltip();

  void RenderIconToTexture(nux::GraphicsEngine&, nux::ObjectPtr<nux::IOpenGLBaseTexture> const&, AbstractLauncherIcon::Ptr const&);

#ifdef NUX_GESTURES_SUPPORT
  virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);
#endif

protected:
  // Introspectable methods
  std::string GetName() const;
  void AddProperties(debug::IntrospectionData&);

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
    ACTION_DRAG_ICON_CANCELLED,
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
  ui::EdgeBarrierSubscriber::Result HandleBarrierEvent(ui::PointerBarrierWrapper::Ptr const& owner, ui::BarrierEvent::Ptr event);
#endif

  void OnExpoChanged();
  void OnSpreadChanged();

  void OnSelectionChanged(AbstractLauncherIcon::Ptr const& selection);

  bool StrutHack();
  bool StartIconDragTimeout(int x, int y);
  bool OnScrollTimeout();

  void SetUrgentTimer(int urgent_wiggle_period);
  void AnimateUrgentIcon(AbstractLauncherIcon::Ptr const& icon);
  void HandleUrgentIcon(AbstractLauncherIcon::Ptr const& icon);
  bool OnUrgentTimeout();

  void SetMousePosition(int x, int y);
  void SetIconUnderMouse(AbstractLauncherIcon::Ptr const& icon);

  void SetStateMouseOverLauncher(bool over_launcher);

  bool MouseBeyondDragThreshold() const;

  void OnDragWindowAnimCompleted();

  bool IconDrawEdgeOnly(AbstractLauncherIcon::Ptr const& icon) const;

  void SetActionState(LauncherActionState actionstate);
  LauncherActionState GetActionState() const;

  void EnsureScrollTimer();
  bool MouseOverTopScrollArea();
  bool MouseOverBottomScrollArea();

  float DragOutProgress() const;
  float IconUrgentPulseValue(AbstractLauncherIcon::Ptr const& icon) const;
  float IconPulseOnceValue(AbstractLauncherIcon::Ptr const& icon) const;
  float IconUrgentWiggleValue(AbstractLauncherIcon::Ptr const& icon) const;
  float IconStartingBlinkValue(AbstractLauncherIcon::Ptr const& icon) const;
  float IconStartingPulseValue(AbstractLauncherIcon::Ptr const& icon) const;
  float IconBackgroundIntensity(AbstractLauncherIcon::Ptr const& icon) const;
  float IconProgressBias(AbstractLauncherIcon::Ptr const& icon) const;

  void SetHidden(bool hidden);

  void UpdateChangeInMousePosition(int delta_x, int delta_y);

  void  SetDndDelta(float x, float y, nux::Geometry const& geo);
  float DragLimiter(float x);

  void SetupRenderArg(AbstractLauncherIcon::Ptr const& icon, ui::RenderArg& arg);
  void FillRenderArg(AbstractLauncherIcon::Ptr const& icon,
                     ui::RenderArg& arg,
                     nux::Point3& center,
                     nux::Geometry const& parent_abs_geo,
                     float folding_threshold,
                     float folded_size,
                     float folded_spacing,
                     float autohide_offset,
                     float folded_z_distance,
                     float animation_neg_rads);

  void RenderArgs(std::list<ui::RenderArg> &launcher_args,
                  nux::Geometry& box_geo, float* launcher_alpha, nux::Geometry const& parent_abs_geo, bool& force_show_window);

  void OnIconAdded(AbstractLauncherIcon::Ptr const& icon);
  void OnIconRemoved(AbstractLauncherIcon::Ptr const& icon);
  void OnIconNeedsRedraw(AbstractLauncherIcon::Ptr const& icon, int monitor);
  void SetupIconAnimations(AbstractLauncherIcon::Ptr const& icon);

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

  void OnDPIChanged();
  void LoadTextures();

  LauncherModel::Ptr model_;
  MockableBaseWindow* parent_;
  ui::AbstractIconRenderer::Ptr icon_renderer_;
  nux::ObjectPtr<nux::View> active_tooltip_;
  std::set<AbstractLauncherIcon::Ptr> animating_urgent_icons_;

  // used by keyboard/a11y-navigation
  AbstractLauncherIcon::Ptr icon_under_mouse_;
  AbstractLauncherIcon::Ptr icon_mouse_down_;
  AbstractLauncherIcon::Ptr drag_icon_;
  AbstractLauncherIcon::Ptr dnd_hovered_icon_;

  bool hovered_;
  bool hidden_;
  bool folded_;
  bool render_drag_window_;
  bool shortcuts_shown_;
  bool data_checked_;
  bool steal_drag_;
  bool drag_edge_touching_;
  bool initial_drag_animation_;
  bool dash_is_open_;
  bool hud_is_open_;

  LauncherActionState launcher_action_state_;

  RawPixel icon_size_;
  int dnd_delta_y_;
  int dnd_delta_x_;
  int postreveal_mousemove_delta_x_;
  int postreveal_mousemove_delta_y_;
  int launcher_drag_delta_;
  int launcher_drag_delta_max_;
  int launcher_drag_delta_min_;
  int enter_x_;
  int enter_y_;
  int last_button_press_;
  int drag_icon_position_;
  int urgent_animation_period_;
  bool urgent_ack_needed_;
  float drag_out_delta_x_;
  bool drag_gesture_ongoing_;
  float last_reveal_progress_;

  nux::Point mouse_position_;
  LauncherDragWindow::Ptr drag_window_;
  LauncherHideMachine hide_machine_;
  LauncherHoverMachine hover_machine_;
  TooltipManager tooltip_manager_;

  unity::DndData dnd_data_;
  nux::DndAction drag_action_;

  BaseTexturePtr launcher_sheen_;
  BaseTexturePtr launcher_pressure_effect_;
  BackgroundEffectHelper bg_effect_helper_;

  LauncherPosition launcher_position_;
  connection::Wrapper launcher_position_changed_;

  nux::animation::AnimateValue<float> auto_hide_animation_;
  nux::animation::AnimateValue<float> hover_animation_;
  nux::animation::AnimateValue<float> drag_over_animation_;
  nux::animation::AnimateValue<float> drag_out_animation_;
  nux::animation::AnimateValue<float> drag_icon_animation_;
  nux::animation::AnimateValue<float> dnd_hide_animation_;
  nux::animation::AnimateValue<float> dash_showing_animation_;

  UBusManager ubus_;
  glib::SourceManager sources_;
  EMConverter::Ptr cv_;

  friend class TestLauncher;
};

}
}

#endif // LAUNCHER_H
