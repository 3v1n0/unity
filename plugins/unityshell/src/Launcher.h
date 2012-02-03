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

#include <sys/time.h>

#include <Nux/View.h>
#include <Nux/BaseWindow.h>
#include <Nux/TimerProc.h>
#include <NuxGraphics/IOpenGLAsmShader.h>

#include "PointerBarrier.h"
#include "AbstractIconRenderer.h"
#include "BackgroundEffectHelper.h"
#include "DNDCollectionWindow.h"
#include "DndData.h"
#include "GeisAdapter.h"
#include "Introspectable.h"
#include "LauncherOptions.h"
#include "LauncherDragWindow.h"
#include "LauncherHideMachine.h"
#include "LauncherHoverMachine.h"
#include "UBusWrapper.h"

#define ANIM_DURATION_SHORT_SHORT 100
#define ANIM_DURATION_SHORT 125
#define ANIM_DURATION       200
#define ANIM_DURATION_LONG  350

#define MAX_SUPERKEY_LABELS 10

#define START_DRAGICON_DURATION 250

class QuicklistView;

namespace unity
{
namespace launcher
{
class AbstractLauncherIcon;
class LauncherModel;

class Launcher : public unity::debug::Introspectable, public nux::View
{
  NUX_DECLARE_OBJECT_TYPE(Launcher, nux::View);
public:

  Launcher(nux::BaseWindow* parent, NUX_FILE_LINE_PROTO);
  ~Launcher();

  nux::Property<Display*> display;
  nux::Property<int> monitor;
  nux::Property<Options::Ptr> options;

  virtual void Draw(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw);
  virtual void PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw);

  AbstractLauncherIcon* GetSelectedMenuIcon() const;

  void SetIconSize(int tile_size, int icon_size);

  LauncherHideMachine* HideMachine() { return _hide_machine; }

  bool Hidden() const
  {
    return _hidden;
  }

  void ForceReveal(bool force);
  void ShowShortcuts(bool show);

  void SetModel(LauncherModel* model);
  LauncherModel* GetModel() const;

  void StartKeyShowLauncher();
  void EndKeyShowLauncher();

  void EnsureIconOnScreen(AbstractLauncherIcon* icon);

  void SetBacklightMode(BacklightMode mode);
  BacklightMode GetBacklightMode() const;
  bool IsBackLightModeToggles() const;

  nux::BaseWindow* GetParent() const
  {
    return _parent;
  };

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

  void CheckWindowOverLauncher();
  void EnableCheckWindowOverLauncher(gboolean enabled);

  sigc::signal<void, char*, AbstractLauncherIcon*> launcher_addrequest;
  sigc::signal<void, std::string const&, AbstractLauncherIcon*, std::string const&, std::string const&> launcher_addrequest_special;
  sigc::signal<void, AbstractLauncherIcon*> launcher_removerequest;
  sigc::signal<void> selection_change;
  sigc::signal<void> hidden_changed;

  virtual bool InspectKeyEvent(unsigned int eventType,
                               unsigned int keysym,
                               const char* character);

  void EnterKeyNavMode();
  void ExitKeyNavMode();
  bool IsInKeyNavMode() const;

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

  void OnOptionsChanged(Options::Ptr options);
  void OnOptionChanged();
  void UpdateOptions(Options::Ptr options);

  void OnWindowMaybeIntellihide(guint32 xid);
  void OnWindowMaybeIntellihideDelayed(guint32 xid);
  static gboolean CheckWindowOverLauncherSync(Launcher* self);
  void OnWindowMapped(guint32 xid);
  void OnWindowUnmapped(guint32 xid);

  void OnDragStart(GeisAdapter::GeisDragData* data);
  void OnDragUpdate(GeisAdapter::GeisDragData* data);
  void OnDragFinish(GeisAdapter::GeisDragData* data);

  void OnPointerBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event);

  void OnPluginStateChanged();

  void OnSelectionChanged(AbstractLauncherIcon* selection);

  void OnViewPortSwitchStarted();
  void OnViewPortSwitchEnded();

  static gboolean AnimationTimeout(gpointer data);
  static gboolean StrutHack(gpointer data);
  static gboolean StartIconDragTimeout(gpointer data);

  void SetMousePosition(int x, int y);

  void SetStateMouseOverLauncher(bool over_launcher);

  bool MouseBeyondDragThreshold() const;

  void OnDragWindowAnimCompleted();

  bool IconNeedsAnimation(AbstractLauncherIcon* icon, struct timespec const& current) const;
  bool IconDrawEdgeOnly(AbstractLauncherIcon* icon) const;
  bool AnimationInProgress() const;

  void SetActionState(LauncherActionState actionstate);
  LauncherActionState GetActionState() const;

  void EnsureAnimation();
  void EnsureScrollTimer();

  bool MouseOverTopScrollArea();
  bool MouseOverTopScrollExtrema();

  bool MouseOverBottomScrollArea();
  bool MouseOverBottomScrollExtrema();

  static gboolean OnScrollTimeout(gpointer data);
  static gboolean OnUpdateDragManagerTimeout(gpointer data);

  float DnDStartProgress(struct timespec const& current) const;
  float DnDExitProgress(struct timespec const& current) const;
  float GetHoverProgress(struct timespec const& current) const;
  float AutohideProgress(struct timespec const& current) const;
  float DragThresholdProgress(struct timespec const& current) const;
  float DragHideProgress(struct timespec const& current) const;
  float DragOutProgress(struct timespec const& current) const;
  float IconDesatValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconPresentProgress(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconUrgentProgress(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconShimmerProgress(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconUrgentPulseValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconPulseOnceValue(AbstractLauncherIcon *icon, struct timespec const &current) const;
  float IconUrgentWiggleValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconStartingBlinkValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconStartingPulseValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconBackgroundIntensity(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconProgressBias(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconDropDimValue(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconCenterTransitionProgress(AbstractLauncherIcon* icon, struct timespec const& current) const;
  float IconVisibleProgress(AbstractLauncherIcon* icon, struct timespec const& current) const;

  void SetHover(bool hovered);
  void SetHidden(bool hidden);

  void  SetDndDelta(float x, float y, nux::Geometry const& geo, timespec const& current);
  float DragLimiter(float x);

  void SetupRenderArg(AbstractLauncherIcon* icon, struct timespec const& current, ui::RenderArg& arg);
  void FillRenderArg(AbstractLauncherIcon* icon,
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

  void OnIconAdded(AbstractLauncherIcon* icon);
  void OnIconRemoved(AbstractLauncherIcon* icon);
  void OnOrderChanged();

  void OnIconNeedsRedraw(AbstractLauncherIcon* icon);

  void OnPlaceViewHidden(GVariant* data);
  void OnPlaceViewShown(GVariant* data);

  void DesaturateIcons();
  void SaturateIcons();

  void OnBGColorChanged(GVariant *data);

  void OnLockHideChanged(GVariant *data);

  void OnActionDone(GVariant* data);

  void RenderIconToTexture(nux::GraphicsEngine& GfxContext, AbstractLauncherIcon* icon, nux::ObjectPtr<nux::IOpenGLBaseTexture> texture);

  AbstractLauncherIcon* MouseIconIntersection(int x, int y);
  void EventLogic();
  void MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);
  void MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags);

  void StartIconDragRequest(int x, int y);
  void StartIconDrag(AbstractLauncherIcon* icon);
  void EndIconDrag();
  void UpdateDragWindowPosition(int x, int y);

  float GetAutohidePositionMin() const;
  float GetAutohidePositionMax() const;

  virtual void PreLayoutManagement();
  virtual long PostLayoutManagement(long LayoutResult);
  virtual void PositionChildLayout(float offsetX, float offsetY);

  void SetOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture);
  void RestoreSystemRenderTarget();

  void OnDisplayChanged(Display* display);
  void OnDNDDataCollected(const std::list<char*>& mimes);
  
  void DndReset();
  void DndHoveredIconReset();

  nux::HLayout* m_Layout;

  // used by keyboard/a11y-navigation
  AbstractLauncherIcon* _icon_under_mouse;
  AbstractLauncherIcon* _icon_mouse_down;
  AbstractLauncherIcon* _drag_icon;

  QuicklistView* _active_quicklist;

  bool  _hovered;
  bool  _hidden;
  bool  _render_drag_window;
  bool  _check_window_over_launcher;

  bool          _shortcuts_shown;

  BacklightMode _backlight_mode;

  float _folded_angle;
  float _neg_folded_angle;
  float _folded_z_distance;
  float _launcher_top_y;
  float _launcher_bottom_y;
  float _edge_overcome_pressure;

  LauncherHideMode _hidemode;

  LauncherActionState _launcher_action_state;
  LaunchAnimation _launch_animation;
  UrgentAnimation _urgent_animation;

  nux::ObjectPtr<nux::IOpenGLBaseTexture> _offscreen_drag_texture;

  ui::PointerBarrierWrapper::Ptr _pointer_barrier;
  ui::Decaymulator::Ptr decaymulator_;

  int _space_between_icons;
  int _icon_size;
  int _icon_image_size;
  int _icon_image_size_delta;
  int _icon_glow_size;
  int _dnd_delta_y;
  int _dnd_delta_x;
  int _postreveal_mousemove_delta_x;
  int _postreveal_mousemove_delta_y;
  int _launcher_drag_delta;
  int _enter_y;
  int _last_button_press;
  int _drag_out_id;
  float _drag_out_delta_x;
  float _background_alpha;

  guint _autoscroll_handle;
  guint _start_dragicon_handle;
  guint _dnd_check_handle;

  nux::Point2   _mouse_position;
  nux::BaseWindow* _parent;
  LauncherModel* _model;
  LauncherDragWindow* _drag_window;
  LauncherHideMachine* _hide_machine;
  LauncherHoverMachine* _hover_machine;

  unity::DndData _dnd_data;
  nux::DndAction    _drag_action;
  bool              _data_checked;
  bool              _steal_drag;
  bool              _drag_edge_touching;
  AbstractLauncherIcon*     _dnd_hovered_icon;
  unity::DNDCollectionWindow* _collection_window;
  sigc::connection _on_data_collected_connection;

  Atom              _selection_atom;

  guint             _launcher_animation_timeout;

  /* gdbus */
  guint                       _dbus_owner;
  static const gchar          introspection_xml[];
  static GDBusInterfaceVTable interface_vtable;

  static void OnBusAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
  static void OnNameAcquired(GDBusConnection* connection, const gchar* name, gpointer user_data);
  static void OnNameLost(GDBusConnection* connection, const gchar* name, gpointer user_data);
  static void handle_dbus_method_call(GDBusConnection*       connection,
                                      const gchar*           sender,
                                      const gchar*           object_path,
                                      const gchar*           interface_name,
                                      const gchar*           method_name,
                                      GVariant*              parameters,
                                      GDBusMethodInvocation* invocation,
                                      gpointer               user_data);

  struct timespec  _times[TIME_LAST];

  bool _initial_drag_animation;

  UBusManager ubus;

  nux::Color _background_color;
  BaseTexturePtr   launcher_sheen_;
  bool _dash_is_open;

  ui::AbstractIconRenderer::Ptr icon_renderer;
  BackgroundEffectHelper bg_effect_helper_;
};

}
}

#endif // LAUNCHER_H
