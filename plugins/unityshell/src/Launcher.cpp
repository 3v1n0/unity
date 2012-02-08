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

#include "config.h"
#include <math.h>

#include <Nux/Nux.h>
#include <Nux/VScrollBar.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <Nux/MenuPage.h>
#include <NuxCore/Logger.h>

#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GpuDevice.h>
#include <NuxGraphics/GLTextureResourceManager.h>

#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "Launcher.h"
#include "AbstractLauncherIcon.h"
#include "SpacerLauncherIcon.h"
#include "LauncherModel.h"
#include "QuicklistManager.h"
#include "QuicklistView.h"
#include "IconRenderer.h"
#include "TimeUtil.h"
#include "WindowManager.h"
#include "UScreen.h"

#include "ubus-server.h"
#include "UBusMessages.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

namespace unity
{
using ui::RenderArg;
using ui::PointerBarrierWrapper;
using ui::Decaymulator;

namespace launcher
{

namespace
{

nux::logging::Logger logger("unity.launcher");

const int URGENT_BLINKS = 3;
const int WIGGLE_CYCLES = 6;

const int MAX_STARTING_BLINKS = 5;
const int STARTING_BLINK_LAMBDA = 3;

const int PULSE_BLINK_LAMBDA = 2;

const float BACKLIGHT_STRENGTH = 0.9f;
const int panel_height = 24;
const int ICON_PADDING = 6;
const int RIGHT_LINE_WIDTH = 1;

}

#define TRIGGER_SQR_RADIUS 25

#define MOUSE_DEADZONE 15

#define DRAG_OUT_PIXELS 300.0f

#define S_DBUS_NAME  "com.canonical.Unity.Launcher"
#define S_DBUS_PATH  "/com/canonical/Unity/Launcher"
#define S_DBUS_IFACE "com.canonical.Unity.Launcher"

// FIXME: key-code defines for Up/Down/Left/Right of numeric keypad - needs to
// be moved to the correct place in NuxGraphics-headers
#define NUX_KP_DOWN  0xFF99
#define NUX_KP_UP    0xFF97
#define NUX_KP_LEFT  0xFF96
#define NUX_KP_RIGHT 0xFF98

NUX_IMPLEMENT_OBJECT_TYPE(Launcher);

const gchar Launcher::introspection_xml[] =
  "<node>"
  "  <interface name='com.canonical.Unity.Launcher'>"
  ""
  "    <method name='AddLauncherItemFromPosition'>"
  "      <arg type='s' name='title' direction='in'/>"
  "      <arg type='s' name='icon' direction='in'/>"
  "      <arg type='i' name='icon_x' direction='in'/>"
  "      <arg type='i' name='icon_y' direction='in'/>"
  "      <arg type='i' name='icon_size' direction='in'/>"
  "      <arg type='s' name='desktop_file' direction='in'/>"
  "      <arg type='s' name='aptdaemon_task' direction='in'/>"
  "    </method>"
  ""
  "  </interface>"
  "</node>";

GDBusInterfaceVTable Launcher::interface_vtable =
{
  Launcher::handle_dbus_method_call,
  NULL,
  NULL
};

Launcher::Launcher(nux::BaseWindow* parent,
                   NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , _model(0)
  , _collection_window(NULL)
  , _background_color(nux::color::DimGray)
  , _dash_is_open(false)
{

  _parent = parent;
  _active_quicklist = nullptr;

  monitor = 0;

  _hide_machine = new LauncherHideMachine();
  _hide_machine->should_hide_changed.connect(sigc::mem_fun(this, &Launcher::SetHidden));
  _hide_machine->reveal_progress.changed.connect([&](float value) -> void { EnsureAnimation(); });

  _hover_machine = new LauncherHoverMachine();
  _hover_machine->should_hover_changed.connect(sigc::mem_fun(this, &Launcher::SetHover));

  _launcher_animation_timeout = 0;

  m_Layout = new nux::HLayout(NUX_TRACKER_LOCATION);

  mouse_down.connect(sigc::mem_fun(this, &Launcher::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &Launcher::RecvMouseUp));
  mouse_drag.connect(sigc::mem_fun(this, &Launcher::RecvMouseDrag));
  mouse_enter.connect(sigc::mem_fun(this, &Launcher::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Launcher::RecvMouseLeave));
  mouse_move.connect(sigc::mem_fun(this, &Launcher::RecvMouseMove));
  mouse_wheel.connect(sigc::mem_fun(this, &Launcher::RecvMouseWheel));
  //OnEndFocus.connect   (sigc::mem_fun (this, &Launcher::exitKeyNavMode));

  CaptureMouseDownAnyWhereElse(true);
  SetAcceptKeyNavFocusOnMouseDown(false);

  QuicklistManager& ql_manager = *(QuicklistManager::Default());
  ql_manager.quicklist_opened.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistOpened));
  ql_manager.quicklist_closed.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistClosed));

  WindowManager& plugin_adapter = *(WindowManager::Default());
  plugin_adapter.window_maximized.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_restored.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_unminimized.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_mapped.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_unmapped.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_shown.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_hidden.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_resized.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_moved.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihide));
  plugin_adapter.window_focus_changed.connect(sigc::mem_fun(this, &Launcher::OnWindowMaybeIntellihideDelayed));
  plugin_adapter.window_mapped.connect(sigc::mem_fun(this, &Launcher::OnWindowMapped));
  plugin_adapter.window_unmapped.connect(sigc::mem_fun(this, &Launcher::OnWindowUnmapped));

  plugin_adapter.initiate_spread.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  plugin_adapter.initiate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  plugin_adapter.terminate_spread.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  plugin_adapter.terminate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  plugin_adapter.compiz_screen_viewport_switch_started.connect(sigc::mem_fun(this, &Launcher::OnViewPortSwitchStarted));
  plugin_adapter.compiz_screen_viewport_switch_ended.connect(sigc::mem_fun(this, &Launcher::OnViewPortSwitchEnded));

  GeisAdapter& adapter = *(GeisAdapter::Default());
  adapter.drag_start.connect(sigc::mem_fun(this, &Launcher::OnDragStart));
  adapter.drag_update.connect(sigc::mem_fun(this, &Launcher::OnDragUpdate));
  adapter.drag_finish.connect(sigc::mem_fun(this, &Launcher::OnDragFinish));

  display.changed.connect(sigc::mem_fun(this, &Launcher::OnDisplayChanged));

  SetCompositionLayout(m_Layout);

  _folded_angle           = 1.0f;
  _neg_folded_angle       = -1.0f;
  _space_between_icons    = 5;
  _launcher_top_y         = 0;
  _launcher_bottom_y      = 0;
  _folded_z_distance      = 10.0f;
  _launcher_action_state  = ACTION_NONE;
  _hidemode               = LAUNCHER_HIDE_NEVER;
  _icon_under_mouse       = NULL;
  _icon_mouse_down        = NULL;
  _drag_icon              = NULL;
  _icon_image_size        = 48;
  _icon_glow_size         = 62;
  _icon_image_size_delta  = 6;
  _icon_size              = _icon_image_size + _icon_image_size_delta;

  _enter_y                = 0;
  _launcher_drag_delta    = 0;
  _dnd_delta_y            = 0;
  _dnd_delta_x            = 0;

  _autoscroll_handle      = 0;
  _start_dragicon_handle  = 0;
  _dnd_check_handle       = 0;
  _last_reveal_progress   = 0;

  _shortcuts_shown        = false;
  _hovered                = false;
  _hidden                 = false;
  _render_drag_window     = false;
  _drag_edge_touching     = false;
  _steal_drag             = false;
  _last_button_press      = 0;
  _selection_atom         = 0;
  _drag_out_id            = 0;
  _drag_out_delta_x       = 0.0f;
  _edge_overcome_pressure = 0.0f;

  // FIXME: remove
  _initial_drag_animation = false;

  _check_window_over_launcher   = true;
  _postreveal_mousemove_delta_x = 0;
  _postreveal_mousemove_delta_y = 0;

  _data_checked = false;
  _collection_window = new unity::DNDCollectionWindow();
  _collection_window->SinkReference();
  _on_data_collected_connection = _collection_window->collected.connect(sigc::mem_fun(this, &Launcher::OnDNDDataCollected));

  // 0 out timers to avoid wonky startups
  int i;
  for (i = 0; i < TIME_LAST; ++i)
  {
    _times[i].tv_sec = 0;
    _times[i].tv_nsec = 0;
  }

  _dnd_hovered_icon = NULL;
  _drag_window = NULL;
  _offscreen_drag_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(2, 2, 1, nux::BITFMT_R8G8B8A8);

  ubus.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &Launcher::OnOverlayShown));
  ubus.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &Launcher::OnOverlayHidden));
  ubus.RegisterInterest(UBUS_LAUNCHER_ACTION_DONE, sigc::mem_fun(this, &Launcher::OnActionDone));
  ubus.RegisterInterest(UBUS_BACKGROUND_COLOR_CHANGED, sigc::mem_fun(this, &Launcher::OnBGColorChanged));
  ubus.RegisterInterest(UBUS_LAUNCHER_LOCK_HIDE, sigc::mem_fun(this, &Launcher::OnLockHideChanged));
  _dbus_owner = g_bus_own_name(G_BUS_TYPE_SESSION,
                               S_DBUS_NAME,
                               (GBusNameOwnerFlags)(G_BUS_NAME_OWNER_FLAGS_REPLACE | G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT),
                               OnBusAcquired,
                               OnNameAcquired,
                               OnNameLost,
                               this,
                               NULL);

  SetDndEnabled(false, true);

  icon_renderer = ui::AbstractIconRenderer::Ptr(new ui::IconRenderer());
  icon_renderer->SetTargetSize(_icon_size, _icon_image_size, _space_between_icons);

  // request the latest colour from bghash
  ubus.SendMessage(UBUS_BACKGROUND_REQUEST_COLOUR_EMIT, NULL);

  SetAcceptMouseWheelEvent(true);

  bg_effect_helper_.owner = this;
  bg_effect_helper_.enabled = false;

  //FIXME (gord)- replace with async loading
  unity::glib::Object<GdkPixbuf> pixbuf;
  unity::glib::Error error;
  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR"/dash_sheen.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/dash_sheen.png" << ": " << error;
  }
  else
  {
    launcher_sheen_ = nux::CreateTexture2DFromPixbuf(pixbuf, true);
    // TODO: when nux has the ability to create a smart pointer that takes
    // ownership without adding a reference, we can remove the unref here.  By
    // unreferencing, the object is solely owned by the smart pointer.
    launcher_sheen_->UnReference();
  }

  pixbuf = gdk_pixbuf_new_from_file(PKGDATADIR"/launcher_pressure_effect.png", &error);
  if (error)
  {
    LOG_WARN(logger) << "Unable to texture " << PKGDATADIR << "/launcher_pressure_effect.png" << ": " << error;
  }
  else
  {
    launcher_pressure_effect_ = nux::CreateTexture2DFromPixbuf(pixbuf, true);
    // TODO: when nux has the ability to create a smart pointer that takes
    // ownership without adding a reference, we can remove the unref here.  By
    // unreferencing, the object is solely owned by the smart pointer.
    launcher_pressure_effect_->UnReference();
  }

  _pointer_barrier = PointerBarrierWrapper::Ptr(new PointerBarrierWrapper());
  _pointer_barrier->barrier_event.connect(sigc::mem_fun(this, &Launcher::OnPointerBarrierEvent));

  decaymulator_ = Decaymulator::Ptr(new Decaymulator());

  options.changed.connect (sigc::mem_fun (this, &Launcher::OnOptionsChanged));
}

Launcher::~Launcher()
{
  g_bus_unown_name(_dbus_owner);

  if (_dnd_check_handle)
    g_source_remove(_dnd_check_handle);
  if (_autoscroll_handle)
    g_source_remove(_autoscroll_handle);
  if (_start_dragicon_handle)
    g_source_remove(_start_dragicon_handle);
  if (_launcher_animation_timeout > 0)
    g_source_remove(_launcher_animation_timeout);

  if (_on_data_collected_connection.connected())
      _on_data_collected_connection.disconnect();

  g_idle_remove_by_data(this);

  if (_collection_window)
    _collection_window->UnReference();

  delete _hover_machine;
  delete _hide_machine;
}

/* Introspection */
std::string
Launcher::GetName() const
{
  return "Launcher";
}

void
Launcher::OnDisplayChanged(Display* display)
{
  _collection_window->display = display;
}

void
Launcher::OnDragStart(GeisAdapter::GeisDragData* data)
{
  if (_drag_out_id && _drag_out_id == data->id)
    return;

  if (data->touches == 4)
  {
    _drag_out_id = data->id;
    if (_hidden)
    {
      _drag_out_delta_x = 0.0f;
    }
    else
    {
      _drag_out_delta_x = DRAG_OUT_PIXELS;
      _hide_machine->SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
    }
  }
}

void
Launcher::OnDragUpdate(GeisAdapter::GeisDragData* data)
{
  if (data->id == _drag_out_id)
  {
    _drag_out_delta_x = CLAMP(_drag_out_delta_x + data->delta_x, 0.0f, DRAG_OUT_PIXELS);
    EnsureAnimation();
  }
}

void
Launcher::OnDragFinish(GeisAdapter::GeisDragData* data)
{
  if (data->id == _drag_out_id)
  {
    if (_drag_out_delta_x >= DRAG_OUT_PIXELS - 90.0f)
      _hide_machine->SetQuirk(LauncherHideMachine::MT_DRAG_OUT, true);
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_OUT], &_times[TIME_DRAG_OUT], ANIM_DURATION_SHORT);
    _drag_out_id = 0;
    EnsureAnimation();
  }
}

void
Launcher::AddProperties(GVariantBuilder* builder)
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  nux::Geometry abs_geo = GetAbsoluteGeometry();

  unity::variant::BuilderWrapper(builder)
  .add("hover-progress", GetHoverProgress(current))
  .add("dnd-exit-progress", DnDExitProgress(current))
  .add("autohide-progress", AutohideProgress(current))
  .add("dnd-delta", _dnd_delta_y)
  .add("hovered", _hovered)
  .add("hidemode", _hidemode)
  .add("hidden", _hidden)
  .add("x", abs_geo.x)
  .add("y", abs_geo.y)
  .add("width", abs_geo.width)
  .add("height", abs_geo.height)
  .add("monitor", monitor())
  .add("quicklist-open", _hide_machine->GetQuirk(LauncherHideMachine::QUICKLIST_OPEN))
  .add("hide-quirks", _hide_machine->DebugHideQuirks().c_str())
  .add("hover-quirks", _hover_machine->DebugHoverQuirks().c_str())
  .add("icon-size", _icon_size);
}

void Launcher::SetMousePosition(int x, int y)
{
  bool beyond_drag_threshold = MouseBeyondDragThreshold();
  _mouse_position = nux::Point2(x, y);

  if (beyond_drag_threshold != MouseBeyondDragThreshold())
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);

  EnsureScrollTimer();
}

void Launcher::SetStateMouseOverLauncher(bool over_launcher)
{
  _hide_machine->SetQuirk(LauncherHideMachine::MOUSE_OVER_LAUNCHER, over_launcher);
  _hide_machine->SetQuirk(LauncherHideMachine::REVEAL_PRESSURE_PASS, false);
  _hover_machine->SetQuirk(LauncherHoverMachine::MOUSE_OVER_LAUNCHER, over_launcher);
}

bool Launcher::MouseBeyondDragThreshold() const
{
  if (GetActionState() == ACTION_DRAG_ICON)
    return _mouse_position.x > GetGeometry().width + _icon_size / 2;
  return false;
}

/* Render Layout Logic */
float Launcher::GetHoverProgress(struct timespec const& current) const
{
  if (_hovered)
    return CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_ENTER])) / (float) ANIM_DURATION, 0.0f, 1.0f);
  else
    return 1.0f - CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_LEAVE])) / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::DnDExitProgress(struct timespec const& current) const
{
  return pow(1.0f - CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_END])) / (float) ANIM_DURATION_LONG, 0.0f, 1.0f), 2);
}

float Launcher::DragOutProgress(struct timespec const& current) const
{
  float timeout = CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_OUT])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  float progress = CLAMP(_drag_out_delta_x / DRAG_OUT_PIXELS, 0.0f, 1.0f);

  if (_drag_out_id || _hide_machine->GetQuirk(LauncherHideMachine::MT_DRAG_OUT))
    return progress;
  return progress * (1.0f - timeout);
}

float Launcher::AutohideProgress(struct timespec const& current) const
{
  // time-based progress (full scale or finish the TRIGGER_AUTOHIDE_MIN -> 0.00f on bfb)
  float animation_progress;
  animation_progress = CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_AUTOHIDE])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  if (_hidden)
    return animation_progress;
  else
    return 1.0f - animation_progress;
}

float Launcher::DragHideProgress(struct timespec const& current) const
{
  if (_drag_edge_touching)
    return CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_EDGE_TOUCH])) / (float)(ANIM_DURATION * 3), 0.0f, 1.0f);
  else
    return 1.0f - CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_EDGE_TOUCH])) / (float)(ANIM_DURATION * 3), 0.0f, 1.0f);
}

float Launcher::DragThresholdProgress(struct timespec const& current) const
{
  if (MouseBeyondDragThreshold())
    return 1.0f - CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_THRESHOLD])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  else
    return CLAMP((float)(unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_THRESHOLD])) / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
}

gboolean Launcher::AnimationTimeout(gpointer data)
{
  Launcher* self = (Launcher*) data;
  self->NeedRedraw();
  self->_launcher_animation_timeout = 0;
  return false;
}

void Launcher::EnsureAnimation()
{
  NeedRedraw();
}

bool Launcher::IconNeedsAnimation(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_VISIBLE);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_RUNNING);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_STARTING);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_URGENT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * URGENT_BLINKS * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PULSE_ONCE);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * PULSE_BLINK_LAMBDA * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PRESENTED);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_SHIMMER);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_LONG)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_CENTER_SAVED);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PROGRESS);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_DROP_DIM);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_DESAT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_DROP_PRELIGHT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  return false;
}

bool Launcher::AnimationInProgress() const
{
  // performance here can be improved by caching the longer remaining animation found and short circuiting to that each time
  // this way extra checks may be avoided

  if (_last_reveal_progress != _hide_machine->reveal_progress)
    return true;

  // short circuit to avoid unneeded calculations
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  // hover in animation
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_ENTER]) < ANIM_DURATION)
    return true;

  // hover out animation
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_LEAVE]) < ANIM_DURATION)
    return true;

  // drag end animation
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_END]) < ANIM_DURATION_LONG)
    return true;

  // hide animation (time only), position is trigger manually on the bfb
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_AUTOHIDE]) < ANIM_DURATION_SHORT)
    return true;

  // collapse animation on DND out of launcher space
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_THRESHOLD]) < ANIM_DURATION_SHORT)
    return true;

  // hide animation for dnd
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_EDGE_TOUCH]) < ANIM_DURATION * 6)
    return true;

  // restore from drag_out animation
  if (unity::TimeUtil::TimeDelta(&current, &_times[TIME_DRAG_OUT]) < ANIM_DURATION_SHORT)
    return true;

  // animations happening on specific icons
  LauncherModel::iterator it;
  for (it = _model->begin(); it != _model->end(); it++)
    if (IconNeedsAnimation(*it, current))
      return true;

  return false;
}

/* Min is when you are on the trigger */
float Launcher::GetAutohidePositionMin() const
{
  if (options()->auto_hide_animation() == SLIDE_ONLY || options()->auto_hide_animation() == FADE_AND_SLIDE)
    return 0.35f;
  else
    return 0.25f;
}
/* Max is the initial state over the bfb */
float Launcher::GetAutohidePositionMax() const
{
  if (options()->auto_hide_animation() == SLIDE_ONLY || options()->auto_hide_animation() == FADE_AND_SLIDE)
    return 1.00f;
  else
    return 0.75f;
}


float Launcher::IconVisibleProgress(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
  {
    struct timespec icon_visible_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_VISIBLE);
    int enter_ms = unity::TimeUtil::TimeDelta(&current, &icon_visible_time);
    return CLAMP((float) enter_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  }
  else
  {
    struct timespec icon_hide_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_VISIBLE);
    int hide_ms = unity::TimeUtil::TimeDelta(&current, &icon_hide_time);
    return 1.0f - CLAMP((float) hide_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  }
}

void Launcher::SetDndDelta(float x, float y, nux::Geometry const& geo, timespec const& current)
{
  AbstractLauncherIcon* anchor = nullptr;
  LauncherModel::iterator it;
  anchor = MouseIconIntersection(x, _enter_y);

  if (anchor)
  {
    float position = y;
    for (it = _model->begin(); it != _model->end(); it++)
    {
      if (*it == anchor)
      {
        position += _icon_size / 2;
        _launcher_drag_delta = _enter_y - position;

        if (position + _icon_size / 2 + _launcher_drag_delta > geo.height)
          _launcher_drag_delta -= (position + _icon_size / 2 + _launcher_drag_delta) - geo.height;

        break;
      }
      position += (_icon_size + _space_between_icons) * IconVisibleProgress(*it, current);
    }
  }
}

float Launcher::IconPresentProgress(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec icon_present_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PRESENTED);
  int ms = unity::TimeUtil::TimeDelta(&current, &icon_present_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_PRESENTED))
    return result;
  else
    return 1.0f - result;
}

float Launcher::IconUrgentProgress(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec urgent_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_URGENT);
  int urgent_ms = unity::TimeUtil::TimeDelta(&current, &urgent_time);
  float result;

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
    result = CLAMP((float) urgent_ms / (float)(ANIM_DURATION_SHORT * WIGGLE_CYCLES), 0.0f, 1.0f);
  else
    result = CLAMP((float) urgent_ms / (float)(ANIM_DURATION_LONG * URGENT_BLINKS * 2), 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT))
    return result;
  else
    return 1.0f - result;
}

float Launcher::IconDropDimValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec dim_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_DROP_DIM);
  int dim_ms = unity::TimeUtil::TimeDelta(&current, &dim_time);
  float result = CLAMP((float) dim_ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_DROP_DIM))
    return 1.0f - result;
  else
    return result;
}

float Launcher::IconDesatValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec dim_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_DESAT);
  int ms = unity::TimeUtil::TimeDelta(&current, &dim_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION_SHORT_SHORT, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_DESAT))
    return 1.0f - result;
  else
    return result;
}

float Launcher::IconShimmerProgress(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec shimmer_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_SHIMMER);
  int shimmer_ms = unity::TimeUtil::TimeDelta(&current, &shimmer_time);
  return CLAMP((float) shimmer_ms / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);
}

float Launcher::IconCenterTransitionProgress(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec save_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_CENTER_SAVED);
  int save_ms = unity::TimeUtil::TimeDelta(&current, &save_time);
  return CLAMP((float) save_ms / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::IconUrgentPulseValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT))
    return 1.0f; // we are full on in a normal condition

  double urgent_progress = (double) IconUrgentProgress(icon, current);
  return 0.5f + (float)(std::cos(M_PI * (float)(URGENT_BLINKS * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconPulseOnceValue(AbstractLauncherIcon *icon, struct timespec const &current) const
{
  struct timespec pulse_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PULSE_ONCE);
  int pulse_ms = unity::TimeUtil::TimeDelta(&current, &pulse_time);
  double pulse_progress = (double) CLAMP((float) pulse_ms / (ANIM_DURATION_LONG * PULSE_BLINK_LAMBDA * 2), 0.0f, 1.0f);

  if (pulse_progress == 1.0f)
    icon->SetQuirk(AbstractLauncherIcon::QUIRK_PULSE_ONCE, false);

  return 0.5f + (float) (std::cos(M_PI * 2.0 * pulse_progress)) * 0.5f;
}

float Launcher::IconUrgentWiggleValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT))
    return 0.0f; // we are full on in a normal condition

  double urgent_progress = (double) IconUrgentProgress(icon, current);
  return 0.3f * (float)(std::sin(M_PI * (float)(WIGGLE_CYCLES * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconStartingBlinkValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec starting_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_STARTING);
  int starting_ms = unity::TimeUtil::TimeDelta(&current, &starting_time);
  double starting_progress = (double) CLAMP((float) starting_ms / (float)(ANIM_DURATION_LONG * STARTING_BLINK_LAMBDA), 0.0f, 1.0f);
  double val = IsBackLightModeToggles() ? 3.0f : 4.0f;
  return 0.5f + (float)(std::cos(M_PI * val * starting_progress)) * 0.5f;
}

float Launcher::IconStartingPulseValue(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec starting_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_STARTING);
  int starting_ms = unity::TimeUtil::TimeDelta(&current, &starting_time);
  double starting_progress = (double) CLAMP((float) starting_ms / (float)(ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2), 0.0f, 1.0f);

  if (starting_progress == 1.0f && !icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING))
  {
    icon->SetQuirk(AbstractLauncherIcon::QUIRK_STARTING, false);
    icon->ResetQuirkTime(AbstractLauncherIcon::QUIRK_STARTING);
  }

  return 0.5f + (float)(std::cos(M_PI * (float)(MAX_STARTING_BLINKS * 2) * starting_progress)) * 0.5f;
}

float Launcher::IconBackgroundIntensity(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  float result = 0.0f;

  struct timespec running_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_RUNNING);
  int running_ms = unity::TimeUtil::TimeDelta(&current, &running_time);
  float running_progress = CLAMP((float) running_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);

  if (!icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING))
    running_progress = 1.0f - running_progress;

  // After we finish a fade in from running, we can reset the quirk
  if (running_progress == 1.0f && icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING))
    icon->SetQuirk(AbstractLauncherIcon::QUIRK_STARTING, false);

  float backlight_strength;
  if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
    backlight_strength = BACKLIGHT_STRENGTH;
  else if (IsBackLightModeToggles())
    backlight_strength = BACKLIGHT_STRENGTH * running_progress;
  else
    backlight_strength = 0.0f;

  switch (options()->launch_animation())
  {
    case LAUNCH_ANIMATION_NONE:
      result = backlight_strength;
      break;
    case LAUNCH_ANIMATION_BLINK:
      if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
        result = IconStartingBlinkValue(icon, current);
      else if (options()->backlight_mode() == BACKLIGHT_ALWAYS_OFF)
        result = 1.0f - IconStartingBlinkValue(icon, current);
      else
        result = backlight_strength; // The blink concept is a failure in this case (it just doesn't work right)
      break;
    case LAUNCH_ANIMATION_PULSE:
      if (running_progress == 1.0f && icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING))
        icon->ResetQuirkTime(AbstractLauncherIcon::QUIRK_STARTING);

      result = backlight_strength;
      if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
        result *= CLAMP(running_progress + IconStartingPulseValue(icon, current), 0.0f, 1.0f);
      else if (IsBackLightModeToggles())
        result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconStartingPulseValue(icon, current));
      else
        result = 1.0f - CLAMP(running_progress + IconStartingPulseValue(icon, current), 0.0f, 1.0f);
      break;
  }

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_PULSE_ONCE))
  {
    if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
      result *= CLAMP(running_progress + IconPulseOnceValue(icon, current), 0.0f, 1.0f);
    else if (options()->backlight_mode() == BACKLIGHT_NORMAL)
      result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconPulseOnceValue(icon, current));
    else
      result = 1.0f - CLAMP(running_progress + IconPulseOnceValue(icon, current), 0.0f, 1.0f);
  }

  // urgent serves to bring the total down only
  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT) && options()->urgent_animation() == URGENT_ANIMATION_PULSE)
    result *= 0.2f + 0.8f * IconUrgentPulseValue(icon, current);

  return result;
}

float Launcher::IconProgressBias(AbstractLauncherIcon* icon, struct timespec const& current) const
{
  struct timespec icon_progress_time = icon->GetQuirkTime(AbstractLauncherIcon::QUIRK_PROGRESS);
  int ms = unity::TimeUtil::TimeDelta(&current, &icon_progress_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_PROGRESS))
    return -1.0f + result;
  else
    return result;
}

bool Launcher::IconDrawEdgeOnly(AbstractLauncherIcon* icon) const
{
  if (options()->backlight_mode() == BACKLIGHT_EDGE_TOGGLE)
    return true;

  if (options()->backlight_mode() == BACKLIGHT_NORMAL_EDGE_TOGGLE && !icon->WindowVisibleOnMonitor(monitor))
    return true;

  return false;
}

void Launcher::SetupRenderArg(AbstractLauncherIcon* icon, struct timespec const& current, RenderArg& arg)
{
  float desat_value = IconDesatValue(icon, current);
  arg.icon                = icon;
  arg.alpha               = 0.5f + 0.5f * desat_value;
  arg.saturation          = desat_value;
  arg.colorify            = nux::color::White;
  arg.running_arrow       = icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING);
  arg.running_colored     = icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT);
  arg.running_on_viewport = icon->WindowVisibleOnMonitor(monitor);
  arg.draw_edge_only      = IconDrawEdgeOnly(icon);
  arg.active_colored      = false;
  arg.x_rotation          = 0.0f;
  arg.y_rotation          = 0.0f;
  arg.z_rotation          = 0.0f;
  arg.skip                = false;
  arg.stick_thingy        = false;
  arg.keyboard_nav_hl     = false;
  arg.progress_bias       = IconProgressBias(icon, current);
  arg.progress            = CLAMP(icon->GetProgress(), 0.0f, 1.0f);
  arg.draw_shortcut       = _shortcuts_shown && !_hide_machine->GetQuirk(LauncherHideMachine::PLACES_VISIBLE);
  arg.system_item         = icon->Type() == AbstractLauncherIcon::TYPE_HOME;

  if (_dash_is_open)
    arg.active_arrow = icon->Type() == AbstractLauncherIcon::TYPE_HOME;
  else
    arg.active_arrow = icon->GetQuirk(AbstractLauncherIcon::QUIRK_ACTIVE);

  guint64 shortcut = icon->GetShortcut();
  if (shortcut > 32)
    arg.shortcut_label = (char) shortcut;
  else
    arg.shortcut_label = 0;

  // we dont need to show strays
  if (!icon->GetQuirk(AbstractLauncherIcon::QUIRK_RUNNING))
  {
    if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT))
    {
      arg.running_arrow = true;
      arg.window_indicators = 1;
    }
    else
      arg.window_indicators = 0;
  }
  else
  {
    arg.window_indicators = std::max<int> (icon->WindowsForMonitor(monitor).size(), 1);
  }

  arg.backlight_intensity = IconBackgroundIntensity(icon, current);
  arg.shimmer_progress = IconShimmerProgress(icon, current);

  float urgent_progress = IconUrgentProgress(icon, current);

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT))
    urgent_progress = CLAMP(urgent_progress * 3.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle
  else
    urgent_progress = CLAMP(urgent_progress * 3.0f - 2.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle
  arg.glow_intensity = urgent_progress;

  if (icon->GetQuirk(AbstractLauncherIcon::QUIRK_URGENT) && options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
  {
    arg.z_rotation = IconUrgentWiggleValue(icon, current);
  }

  if (IsInKeyNavMode())
  {
    if (icon == _model->Selection())
      arg.keyboard_nav_hl = true;
  }
}

void Launcher::FillRenderArg(AbstractLauncherIcon* icon,
                             RenderArg& arg,
                             nux::Point3& center,
                             nux::Geometry const& parent_abs_geo,
                             float folding_threshold,
                             float folded_size,
                             float folded_spacing,
                             float autohide_offset,
                             float folded_z_distance,
                             float animation_neg_rads,
                             struct timespec const& current)
{
  SetupRenderArg(icon, current, arg);

  // reset z
  center.z = 0;

  float size_modifier = IconVisibleProgress(icon, current);
  if (size_modifier < 1.0f)
  {
    arg.alpha *= size_modifier;
    center.z = 300.0f * (1.0f - size_modifier);
  }

  float drop_dim_value = 0.2f + 0.8f * IconDropDimValue(icon, current);

  if (drop_dim_value < 1.0f)
    arg.alpha *= drop_dim_value;

  if (icon == _drag_icon)
  {
    if (MouseBeyondDragThreshold())
      arg.stick_thingy = true;

    if (GetActionState() == ACTION_DRAG_ICON ||
        (_drag_window && _drag_window->Animating()) ||
        icon->IsSpacer())
      arg.skip = true;
    size_modifier *= DragThresholdProgress(current);
  }

  if (size_modifier <= 0.0f)
    arg.skip = true;

  // goes for 0.0f when fully unfolded, to 1.0f folded
  float folding_progress = CLAMP((center.y + _icon_size - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
  float present_progress = IconPresentProgress(icon, current);

  folding_progress *= 1.0f - present_progress;

  float half_size = (folded_size / 2.0f) + (_icon_size / 2.0f - folded_size / 2.0f) * (1.0f - folding_progress);
  float icon_hide_offset = autohide_offset;

  icon_hide_offset *= 1.0f - (present_progress * icon->PresentUrgency());

  // icon is crossing threshold, start folding
  center.z += folded_z_distance * folding_progress;
  arg.x_rotation = animation_neg_rads * folding_progress;

  float spacing_overlap = CLAMP((float)(center.y + (2.0f * half_size * size_modifier) + (_space_between_icons * size_modifier) - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
  float spacing = (_space_between_icons * (1.0f - spacing_overlap) + folded_spacing * spacing_overlap) * size_modifier;

  nux::Point3 centerOffset;
  float center_transit_progress = IconCenterTransitionProgress(icon, current);
  if (center_transit_progress <= 1.0f)
  {
    centerOffset.y = (icon->GetSavedCenter(monitor).y - (center.y + (half_size * size_modifier))) * (1.0f - center_transit_progress);
  }

  center.y += half_size * size_modifier;   // move to center

  arg.render_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y + centerOffset.y), roundf(center.z));
  arg.logical_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y), roundf(center.z));

  icon->SetCenter(nux::Point3(roundf(center.x), roundf(center.y), roundf(center.z)), monitor, parent_abs_geo);

  // FIXME: this is a hack, we should have a look why SetAnimationTarget is necessary in SetAnimationTarget
  // we should ideally just need it at start to set the target
  if (!_initial_drag_animation && icon == _drag_icon && _drag_window && _drag_window->Animating())
    _drag_window->SetAnimationTarget((int) center.x, (int) center.y + _parent->GetGeometry().y);

  center.y += (half_size * size_modifier) + spacing;   // move to end
}

float Launcher::DragLimiter(float x)
{
  float result = (1 - std::pow(159.0 / 160,  std::abs(x))) * 160;

  if (x >= 0.0f)
    return result;
  return -result;
}

nux::Color FullySaturateColor (nux::Color color)
{
  float max = std::max<float>(color.red, std::max<float>(color.green, color.blue));
  color = color * (1.0f / max);
  return color;
}

void Launcher::RenderArgs(std::list<RenderArg> &launcher_args,
                          nux::Geometry& box_geo, float* launcher_alpha, nux::Geometry const& parent_abs_geo)
{
  nux::Geometry geo = GetGeometry();
  LauncherModel::iterator it;
  nux::Point3 center;
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  nux::Color colorify = FullySaturateColor(_background_color);

  float hover_progress = GetHoverProgress(current);
  float folded_z_distance = _folded_z_distance * (1.0f - hover_progress);
  float animation_neg_rads = _neg_folded_angle * (1.0f - hover_progress);

  float folding_constant = 0.25f;
  float folding_not_constant = folding_constant + ((1.0f - folding_constant) * hover_progress);

  float folded_size = _icon_size * folding_not_constant;
  float folded_spacing = _space_between_icons * folding_not_constant;

  center.x = geo.width / 2;
  center.y = _space_between_icons;
  center.z = 0;

  int launcher_height = geo.height;

  // compute required height of launcher AND folding threshold
  float sum = 0.0f + center.y;
  float folding_threshold = launcher_height - _icon_size / 2.5f;
  for (it = _model->begin(); it != _model->end(); it++)
  {
    float height = (_icon_size + _space_between_icons) * IconVisibleProgress(*it, current);
    sum += height;

    // magic constant must some day be explained, for now suffice to say this constant prevents the bottom from "marching";
    float magic_constant = 1.3f;

    float present_progress = IconPresentProgress(*it, current);
    folding_threshold -= CLAMP(sum - launcher_height, 0.0f, height * magic_constant) * (folding_constant + (1.0f - folding_constant) * present_progress);
  }

  if (sum - _space_between_icons <= launcher_height)
    folding_threshold = launcher_height;

  float autohide_offset = 0.0f;
  *launcher_alpha = 1.0f;
  if (_hidemode != LAUNCHER_HIDE_NEVER || _hide_machine->GetQuirk(LauncherHideMachine::LOCK_HIDE))
  {

    float autohide_progress = AutohideProgress(current) * (1.0f - DragOutProgress(current));
    if (options()->auto_hide_animation() == FADE_ONLY)
    {
      *launcher_alpha = 1.0f - autohide_progress;
    }
    else
    {
      if (autohide_progress > 0.0f)
      {
        autohide_offset -= geo.width * autohide_progress;
        if (options()->auto_hide_animation() == FADE_AND_SLIDE)
          *launcher_alpha = 1.0f - 0.5f * autohide_progress;
      }
    }
  }

  float drag_hide_progress = DragHideProgress(current);
  if (_hidemode != LAUNCHER_HIDE_NEVER && drag_hide_progress > 0.0f)
  {
    autohide_offset -= geo.width * 0.25f * drag_hide_progress;

    if (drag_hide_progress >= 1.0f)
      _hide_machine->SetQuirk(LauncherHideMachine::DND_PUSHED_OFF, true);
  }

  // Inform the painter where to paint the box
  box_geo = geo;

  if (_hidemode != LAUNCHER_HIDE_NEVER || _hide_machine->GetQuirk(LauncherHideMachine::LOCK_HIDE))
    box_geo.x += autohide_offset;

  /* Why we need last_geo? It stores the last box_geo (note: as it is a static variable,
   * it is initialized only first time). Infact we call SetDndDelta that calls MouseIconIntersection
   * that uses values (HitArea) that are computed in UpdateIconXForm.
   * The problem is that in DrawContent we calls first RenderArgs, then UpdateIconXForm. Just
   * use last_geo to hack this problem.
   */
  static nux::Geometry last_geo = box_geo;

  // this happens on hover, basically its a flag and a value in one, we translate this into a dnd offset
  if (_enter_y != 0 && _enter_y + _icon_size / 2 > folding_threshold)
    SetDndDelta(last_geo.x + last_geo.width / 2, center.y, geo, current);

  // Update the last_geo value.
  last_geo = box_geo;
  _enter_y = 0;

  if (hover_progress > 0.0f && _launcher_drag_delta != 0)
  {
    float delta_y = _launcher_drag_delta;

    // logically dnd exit only restores to the clamped ranges
    // hover_progress restores to 0
    float max = 0.0f;
    float min = MIN(0.0f, launcher_height - sum);

    if (_launcher_drag_delta > max)
      delta_y = max + DragLimiter(delta_y - max);
    else if (_launcher_drag_delta < min)
      delta_y = min + DragLimiter(delta_y - min);

    if (GetActionState() != ACTION_DRAG_LAUNCHER)
    {
      float dnd_progress = DnDExitProgress(current);

      if (_launcher_drag_delta > max)
        delta_y = max + (delta_y - max) * dnd_progress;
      else if (_launcher_drag_delta < min)
        delta_y = min + (delta_y - min) * dnd_progress;

      if (dnd_progress == 0.0f)
        _launcher_drag_delta = (int) delta_y;
    }

    delta_y *= hover_progress;
    center.y += delta_y;
    folding_threshold += delta_y;
  }
  else
  {
    _launcher_drag_delta = 0;
  }

  // The functional position we wish to represent for these icons is not smooth. Rather than introducing
  // special casing to represent this, we use MIN/MAX functions. This helps ensure that even though our
  // function is not smooth it is continuous, which is more important for our visual representation (icons
  // wont start jumping around).  As a general rule ANY if () statements that modify center.y should be seen
  // as bugs.
  int index = 1;
  for (it = _model->main_begin(); it != _model->main_end(); it++)
  {
    RenderArg arg;
    AbstractLauncherIcon* icon = *it;

    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads, current);
    arg.colorify = colorify;
    launcher_args.push_back(arg);
    index++;
  }

  // compute maximum height of shelf
  float shelf_sum = 0.0f;
  for (it = _model->shelf_begin(); it != _model->shelf_end(); it++)
  {
    float height = (_icon_size + _space_between_icons) * IconVisibleProgress(*it, current);
    shelf_sum += height;
  }

  // add bottom padding
  if (shelf_sum > 0.0f)
    shelf_sum += _space_between_icons;

  float shelf_delta = MAX(((launcher_height - shelf_sum) + _space_between_icons) - center.y, 0.0f);
  folding_threshold += shelf_delta;
  center.y += shelf_delta;

  for (it = _model->shelf_begin(); it != _model->shelf_end(); it++)
  {
    RenderArg arg;
    AbstractLauncherIcon* icon = *it;

    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads, current);

    launcher_args.push_back(arg);
  }
}

/* End Render Layout Logic */

void Launcher::ForceReveal(bool force_reveal)
{
  _hide_machine->SetQuirk(LauncherHideMachine::TRIGGER_BUTTON_SHOW, force_reveal);
}

void Launcher::ShowShortcuts(bool show)
{
  _shortcuts_shown = show;
  _hover_machine->SetQuirk(LauncherHoverMachine::SHORTCUT_KEYS_VISIBLE, show);
}

void Launcher::OnBGColorChanged(GVariant *data)
{
  double red = 0.0f, green = 0.0f, blue = 0.0f, alpha = 0.0f;

  g_variant_get(data, "(dddd)", &red, &green, &blue, &alpha);
  _background_color = nux::Color(red, green, blue, alpha);
  NeedRedraw();
}

void Launcher::OnLockHideChanged(GVariant *data)
{
  gboolean enable_lock = FALSE;
  g_variant_get(data, "(b)", &enable_lock);

  if (enable_lock)
  {
    _hide_machine->SetQuirk(LauncherHideMachine::LOCK_HIDE, true);
  }
  else
  {
    _hide_machine->SetQuirk(LauncherHideMachine::LOCK_HIDE, false);
  }
}

void Launcher::DesaturateIcons()
{
  for (auto icon : *_model)
  {
    if (icon->Type () != AbstractLauncherIcon::TYPE_HOME)
      icon->SetQuirk(AbstractLauncherIcon::QUIRK_DESAT, true);
    icon->HideTooltip();
  }
}

void Launcher::SaturateIcons()
{
  for (auto icon : *_model)
  {
    icon->SetQuirk(AbstractLauncherIcon::QUIRK_DESAT, false);
  }
}

void Launcher::OnOverlayShown(GVariant* data)
{
  // check the type of overlay
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, 
                &overlay_identity, &can_maximise, &overlay_monitor);


  if (!g_strcmp0(overlay_identity, "dash"))
  {
    if (overlay_monitor == monitor)
    {
      LauncherModel::iterator it;

      _dash_is_open = true;
      bg_effect_helper_.enabled = true;
      _hide_machine->SetQuirk(LauncherHideMachine::PLACES_VISIBLE, true);
      _hover_machine->SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, true);

      DesaturateIcons();
    }
  }
}

void Launcher::OnOverlayHidden(GVariant* data)
{
  // check the type of overlay
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING, 
                &overlay_identity, &can_maximise, &overlay_monitor);

  if (!g_strcmp0(overlay_identity, "dash"))
  {
    if (!_dash_is_open)
      return;
    
    LauncherModel::iterator it;

    _dash_is_open = false;
    bg_effect_helper_.enabled = false;
    _hide_machine->SetQuirk(LauncherHideMachine::PLACES_VISIBLE, false);
    _hover_machine->SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, false);

    // as the leave event is no more received when the place is opened
    // FIXME: remove when we change the mouse grab strategy in nux
    nux::Point pt = nux::GetWindowCompositor().GetMousePosition();

    SetStateMouseOverLauncher(GetAbsoluteGeometry().IsInside(pt));

    SaturateIcons();
  }
}

void Launcher::OnActionDone(GVariant* data)
{
  _hide_machine->SetQuirk(LauncherHideMachine::LAST_ACTION_ACTIVATE, true);
}

void Launcher::SetHidden(bool hidden)
{
  if (hidden == _hidden)
    return;

  _hidden = hidden;
  _hide_machine->SetQuirk(LauncherHideMachine::LAUNCHER_HIDDEN, hidden);
  _hover_machine->SetQuirk(LauncherHoverMachine::LAUNCHER_HIDDEN, hidden);

  _hide_machine->SetQuirk(LauncherHideMachine::LAST_ACTION_ACTIVATE, false);

  if (hidden)  
  {
    _hide_machine->SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, false);
    _hide_machine->SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
    SetStateMouseOverLauncher(false);
  }

  _postreveal_mousemove_delta_x = 0;
  _postreveal_mousemove_delta_y = 0;

  TimeUtil::SetTimeStruct(&_times[TIME_AUTOHIDE], &_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT);

  _parent->EnableInputWindow(!hidden, "launcher", false, false);

  if (!hidden && GetActionState() == ACTION_DRAG_EXTERNAL)
    DndReset();

  EnsureAnimation();

  hidden_changed.emit();
}

int
Launcher::GetMouseX() const
{
  return _mouse_position.x;
}

int
Launcher::GetMouseY() const
{
  return _mouse_position.y;
}

void
Launcher::EnableCheckWindowOverLauncher(gboolean enabled)
{
  _check_window_over_launcher = enabled;
}

void
Launcher::CheckWindowOverLauncher()
{
  bool any = false;
  bool active = false;

  // state has no mean right now, the check will be done again later
  if (!_check_window_over_launcher)
    return;

  WindowManager::Default()->CheckWindowIntersections(GetAbsoluteGeometry(), active, any);

  _hide_machine->SetQuirk(LauncherHideMachine::ANY_WINDOW_UNDER, any);
  _hide_machine->SetQuirk(LauncherHideMachine::ACTIVE_WINDOW_UNDER, active);
}

gboolean
Launcher::OnUpdateDragManagerTimeout(gpointer data)
{
  Launcher* self = (Launcher*) data;

  if (self->display() == 0)
    return false;

  if (!self->_selection_atom)
    self->_selection_atom = XInternAtom(self->display(), "XdndSelection", false);

  Window drag_owner = XGetSelectionOwner(self->display(), self->_selection_atom);

  // evil hack because Qt does not release the seelction owner on drag finished
  Window root_r, child_r;
  int root_x_r, root_y_r, win_x_r, win_y_r;
  unsigned int mask;
  XQueryPointer(self->display(), DefaultRootWindow(self->display()), &root_r, &child_r, &root_x_r, &root_y_r, &win_x_r, &win_y_r, &mask);

  if (drag_owner && (mask & (Button1Mask | Button2Mask | Button3Mask)))
  {
    if (self->_data_checked == false)
    {
      self->_data_checked = true;
      self->_collection_window->Collect();
    }

    return true;
  }

  self->_data_checked = false;
  self->_collection_window->PushToBack();
  self->_collection_window->EnableInputWindow(false, "DNDCollectionWindow");

  if (self->_dash_is_open && !self->_hovered)
    self->DesaturateIcons();

  self->DndReset();
  self->_hide_machine->SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
  self->_hide_machine->SetQuirk(LauncherHideMachine::DND_PUSHED_OFF, false);

  self->_dnd_check_handle = 0;
  return false;
}

void
Launcher::OnWindowMapped(guint32 xid)
{
  //CompWindow* window = _screen->findWindow(xid);
  //if (window && window->type() | CompWindowTypeDndMask)
  //{
    if (!_dnd_check_handle)
      _dnd_check_handle = g_timeout_add(200, &Launcher::OnUpdateDragManagerTimeout, this);
  //}
}

void
Launcher::OnWindowUnmapped(guint32 xid)
{
  //CompWindow* window = _screen->findWindow(xid);
  //if (window && window->type() | CompWindowTypeDndMask)
  //{
    if (!_dnd_check_handle)
      _dnd_check_handle = g_timeout_add(200, &Launcher::OnUpdateDragManagerTimeout, this);
  //}
}

// FIXME: remove those 2 for Oneiric
void
Launcher::OnWindowMaybeIntellihide(guint32 xid)
{
  if (_hidemode != LAUNCHER_HIDE_NEVER)
    CheckWindowOverLauncher();
}

void
Launcher::OnWindowMaybeIntellihideDelayed(guint32 xid)
{
  /*
   * Delay to let the other window taking the focus first (otherwise focuschanged
   * is emmited with the root window focus
   */
  if (_hidemode != LAUNCHER_HIDE_NEVER)
    g_idle_add((GSourceFunc)CheckWindowOverLauncherSync, this);
}

gboolean
Launcher::CheckWindowOverLauncherSync(Launcher* self)
{
  self->CheckWindowOverLauncher();
  return FALSE;
}

void
Launcher::OnPluginStateChanged()
{
  _hide_machine->SetQuirk (LauncherHideMachine::EXPO_ACTIVE, WindowManager::Default ()->IsExpoActive ());
  _hide_machine->SetQuirk (LauncherHideMachine::SCALE_ACTIVE, WindowManager::Default ()->IsScaleActive ());

  if (_hidemode == LAUNCHER_HIDE_NEVER)
    return;
}

void
Launcher::OnViewPortSwitchStarted()
{
  /*
   *  don't take into account window over launcher state during
   *  the viewport switch as we can get false positives
   *  (like switching to an empty viewport while grabbing a fullscreen window)
   */
  _check_window_over_launcher = false;
}

void
Launcher::OnViewPortSwitchEnded()
{
  /*
   * compute again the list of all window on the new viewport
   * to decide if we should or not hide the launcher
   */
  _check_window_over_launcher = true;
  CheckWindowOverLauncher();
}

LauncherHideMode Launcher::GetHideMode() const
{
  return _hidemode;
}

/* End Launcher Show/Hide logic */

// Hacks around compiz failing to see the struts because the window was just mapped.
gboolean Launcher::StrutHack(gpointer data)
{
  Launcher* self = (Launcher*) data;
  self->_parent->InputWindowEnableStruts(false);

  if (self->_hidemode == LAUNCHER_HIDE_NEVER)
    self->_parent->InputWindowEnableStruts(true);

  return false;
}

void
Launcher::OnOptionsChanged(Options::Ptr options)
{
   UpdateOptions(options);

   options->option_changed.connect(sigc::mem_fun(this, &Launcher::OnOptionChanged));
}

void
Launcher::OnOptionChanged()
{
  UpdateOptions(options());
}

void
Launcher::UpdateOptions(Options::Ptr options)
{
  SetHideMode(options->hide_mode);
  SetIconSize(options->tile_size, options->icon_size);

  // make the effect half as strong as specified as other values shouldn't scale
  // as quickly as the max velocity multiplier
  float responsiveness_mult = ((options->edge_responsiveness() - 1) * .025) + 1;

  decaymulator_->rate_of_decay = options->edge_decay_rate() * responsiveness_mult;
  _edge_overcome_pressure = options->edge_overcome_pressure() * responsiveness_mult;

  _pointer_barrier->threshold = options->edge_stop_velocity();
  _pointer_barrier->max_velocity_multiplier = options->edge_responsiveness();
  _pointer_barrier->DestroyBarrier();
  _pointer_barrier->ConstructBarrier();

  _hide_machine->reveal_pressure = options->edge_reveal_pressure() * responsiveness_mult;
  _hide_machine->edge_decay_rate = options->edge_decay_rate() * responsiveness_mult;

  EnsureAnimation();
}

void Launcher::SetHideMode(LauncherHideMode hidemode)
{
  if (_hidemode == hidemode)
    return;

  if (hidemode != LAUNCHER_HIDE_NEVER)
  {
    _parent->InputWindowEnableStruts(false);
  }
  else
  {
    _parent->EnableInputWindow(true, "launcher", false, false);
    g_timeout_add(1000, &Launcher::StrutHack, this);
    _parent->InputWindowEnableStruts(true);
  }

  _hidemode = hidemode;
  _hide_machine->SetMode((LauncherHideMachine::HideMode) hidemode);
  EnsureAnimation();
}

BacklightMode Launcher::GetBacklightMode() const
{
  return options()->backlight_mode();
}

bool Launcher::IsBackLightModeToggles() const
{
  switch (options()->backlight_mode()) {
    case BACKLIGHT_NORMAL:
    case BACKLIGHT_EDGE_TOGGLE:
    case BACKLIGHT_NORMAL_EDGE_TOGGLE:
      return true;
    default:
      return false;
  }
}

void
Launcher::SetActionState(LauncherActionState actionstate)
{
  if (_launcher_action_state == actionstate)
    return;

  _launcher_action_state = actionstate;

  _hover_machine->SetQuirk(LauncherHoverMachine::LAUNCHER_IN_ACTION, (actionstate != ACTION_NONE));
}

Launcher::LauncherActionState
Launcher::GetActionState() const
{
  return _launcher_action_state;
}

void Launcher::SetHover(bool hovered)
{

  if (hovered == _hovered)
    return;

  _hovered = hovered;

  if (_hovered)
  {
    _enter_y = (int) _mouse_position.y;
    TimeUtil::SetTimeStruct(&_times[TIME_ENTER], &_times[TIME_LEAVE], ANIM_DURATION);
  }
  else
  {
    TimeUtil::SetTimeStruct(&_times[TIME_LEAVE], &_times[TIME_ENTER], ANIM_DURATION);
  }

  if (_dash_is_open && !_hide_machine->GetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE))
  {
    if (hovered && !_hover_machine->GetQuirk(LauncherHoverMachine::SHORTCUT_KEYS_VISIBLE))
      SaturateIcons();
    else
      DesaturateIcons();
  }

  EnsureAnimation();
}

bool Launcher::MouseOverTopScrollArea()
{
  return _mouse_position.y < panel_height;
}

bool Launcher::MouseOverTopScrollExtrema()
{
  return _mouse_position.y == 0;
}

bool Launcher::MouseOverBottomScrollArea()
{
  return _mouse_position.y > GetGeometry().height - panel_height;
}

bool Launcher::MouseOverBottomScrollExtrema()
{
  return _mouse_position.y == GetGeometry().height - 1;
}

gboolean Launcher::OnScrollTimeout(gpointer data)
{
  Launcher* self = (Launcher*) data;
  nux::Geometry geo = self->GetGeometry();

  if (self->IsInKeyNavMode() || !self->_hovered ||
      self->GetActionState() == ACTION_DRAG_LAUNCHER)
    return TRUE;

  if (self->MouseOverTopScrollArea())
  {
    if (self->MouseOverTopScrollExtrema())
      self->_launcher_drag_delta += 6;
    else
      self->_launcher_drag_delta += 3;
  }
  else if (self->MouseOverBottomScrollArea())
  {
    if (self->MouseOverBottomScrollExtrema())
      self->_launcher_drag_delta -= 6;
    else
      self->_launcher_drag_delta -= 3;
  }

  self->EnsureAnimation();
  return TRUE;
}

void Launcher::EnsureScrollTimer()
{
  bool needed = MouseOverTopScrollArea() || MouseOverBottomScrollArea();

  if (needed && !_autoscroll_handle)
  {
    _autoscroll_handle = g_timeout_add(20, &Launcher::OnScrollTimeout, this);
  }
  else if (!needed && _autoscroll_handle)
  {
    g_source_remove(_autoscroll_handle);
    _autoscroll_handle = 0;
  }
}

void Launcher::SetIconSize(int tile_size, int icon_size)
{
  _icon_size = tile_size;
  _icon_image_size = icon_size;
  _icon_image_size_delta = tile_size - icon_size;
  _icon_glow_size = icon_size + 14;

  icon_renderer->SetTargetSize(_icon_size, _icon_image_size, _space_between_icons);

  Resize();
}

void Launcher::Resize()
{
  UScreen* uscreen = UScreen::GetDefault();
  auto geo = uscreen->GetMonitorGeometry(monitor());

  int width = _icon_size + ICON_PADDING*2 + RIGHT_LINE_WIDTH - 2;
  nux::Geometry new_geometry(geo.x, geo.y + panel_height, width, geo.height - panel_height);
  SetMaximumHeight(new_geometry.height);
  _parent->SetGeometry(new_geometry);
  SetGeometry(new_geometry);

  _pointer_barrier->DestroyBarrier();

  _pointer_barrier->x1 = new_geometry.x;
  _pointer_barrier->x2 = new_geometry.x;
  _pointer_barrier->y1 = new_geometry.y - panel_height;
  _pointer_barrier->y2 = new_geometry.y + new_geometry.height;
  _pointer_barrier->threshold = options()->edge_stop_velocity();

  _pointer_barrier->ConstructBarrier();

}

void Launcher::OnIconAdded(AbstractLauncherIcon* icon)
{
  EnsureAnimation();

  icon->needs_redraw.connect(sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));

  AddChild(icon);
}

void Launcher::OnIconRemoved(AbstractLauncherIcon* icon)
{
  if (icon->needs_redraw_connection.connected())
    icon->needs_redraw_connection.disconnect();

  if (icon == _icon_under_mouse)
    _icon_under_mouse = nullptr;
  if (icon == _icon_mouse_down)
    _icon_mouse_down = nullptr;
  if (icon == _drag_icon)
    _drag_icon = nullptr;

  EnsureAnimation();
  RemoveChild(icon);
}

void Launcher::OnOrderChanged()
{
  EnsureAnimation();
}

void Launcher::SetModel(LauncherModel* model)
{
  _model = model;

  _model->icon_added.connect(sigc::mem_fun(this, &Launcher::OnIconAdded));
  _model->icon_removed.connect(sigc::mem_fun(this, &Launcher::OnIconRemoved));
  _model->order_changed.connect(sigc::mem_fun(this, &Launcher::OnOrderChanged));
  _model->selection_changed.connect(sigc::mem_fun(this, &Launcher::OnSelectionChanged));
}

LauncherModel* Launcher::GetModel() const
{
  return _model;
}

void Launcher::EnsureIconOnScreen(AbstractLauncherIcon* selection)
{
  nux::Point3 center = selection->GetCenter(monitor);
  nux::Geometry geo = GetGeometry();

  int natural_y = 0;
  for (auto icon : *_model)
  {
    if (!icon->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
      continue;

    if (icon == selection)
      break;

    natural_y += _icon_size + _space_between_icons;
  }

  int max_drag_delta = geo.height - (natural_y + _icon_size + (2 * _space_between_icons));
  int min_drag_delta = -natural_y;

  _launcher_drag_delta = std::max<int>(min_drag_delta, std::min<int>(max_drag_delta, _launcher_drag_delta));
}

void Launcher::OnSelectionChanged(AbstractLauncherIcon* selection)
{
  if (IsInKeyNavMode())
  {
    EnsureIconOnScreen(selection);
    EnsureAnimation();
  }
}

void Launcher::OnIconNeedsRedraw(AbstractLauncherIcon* icon)
{
  EnsureAnimation();
}

void Launcher::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void Launcher::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  icon_renderer->monitor = monitor();

  nux::Geometry base = GetGeometry();
  nux::Geometry bkg_box;
  std::list<RenderArg> args;
  std::list<RenderArg>::reverse_iterator rev_it;
  float launcher_alpha = 1.0f;

  // rely on the compiz event loop to come back to us in a nice throttling
  if (AnimationInProgress())
    _launcher_animation_timeout = g_idle_add_full (G_PRIORITY_DEFAULT, &Launcher::AnimationTimeout, this, NULL);

  nux::ROPConfig ROP;
  ROP.Blend = false;
  ROP.SrcBlend = GL_ONE;
  ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::Geometry geo_absolute = GetAbsoluteGeometry();
  RenderArgs(args, bkg_box, &launcher_alpha, geo_absolute);
  bkg_box.width -= RIGHT_LINE_WIDTH;

  if (_drag_icon && _render_drag_window)
  {
    RenderIconToTexture(GfxContext, _drag_icon, _offscreen_drag_texture);
    _drag_window->ShowWindow(true);

    _render_drag_window = false;
  }

  // clear region
  GfxContext.PushClippingRectangle(base);
  gPainter.PushDrawColorLayer(GfxContext, base, nux::Color(0x00000000), true, ROP);

  GfxContext.GetRenderStates().SetBlend(true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
  GfxContext.GetRenderStates().SetColorMask(true, true, true, true);

  int push_count = 1;

  // clip vertically but not horizontally
  GfxContext.PushClippingRectangle(nux::Geometry(base.x, bkg_box.y, base.width, bkg_box.height));

  float reveal_progress = _hide_machine->reveal_progress;
  if (_hidden && reveal_progress > 0 && launcher_pressure_effect_.IsValid())
  {
    if (std::abs(_last_reveal_progress - reveal_progress) <= .1f)
    {
      _last_reveal_progress = reveal_progress;
    }
    else
    {
      if (_last_reveal_progress > reveal_progress)
        _last_reveal_progress -= .1f;
      else
        _last_reveal_progress += .1f;
    }
    nux::Color pressure_color = options()->pressure_color * _last_reveal_progress;
    nux::TexCoordXForm texxform_pressure;
    GfxContext.QRP_1Tex(base.x, base.y, base.width, base.height,
                        launcher_pressure_effect_->GetDeviceTexture(),
                        texxform_pressure,
                        pressure_color);
  }
  else
  {
    _last_reveal_progress = 0;
  }

  if (_dash_is_open)
  {
    if (BackgroundEffectHelper::blur_type != unity::BLUR_NONE && (bkg_box.x + bkg_box.width > 0))
    {
      nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, base.width, base.height);
      auto blur_texture = bg_effect_helper_.GetBlurRegion(blur_geo);

      if (blur_texture.IsValid())
      {
        nux::TexCoordXForm texxform_blur_bg;
        texxform_blur_bg.flip_v_coord = true;
        texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
        texxform_blur_bg.uoffset = ((float) base.x) / geo_absolute.width;
        texxform_blur_bg.voffset = ((float) base.y) / geo_absolute.height;

        GfxContext.PushClippingRectangle(bkg_box);
        gPainter.PushDrawTextureLayer(GfxContext, base,
                                      blur_texture,
                                      texxform_blur_bg,
                                      nux::color::White,
                                      true,
                                      ROP);
        GfxContext.PopClippingRectangle();

        push_count++;
      }
    }

    unsigned int alpha = 0, src = 0, dest = 0;
    GfxContext.GetRenderStates().GetBlend(alpha, src, dest);

    // apply the darkening
    GfxContext.GetRenderStates().SetBlend(true, GL_ZERO, GL_SRC_COLOR);
    gPainter.Paint2DQuadColor(GfxContext, bkg_box, nux::Color(0.7f, 0.7f, 0.7f, 1.0f));
    GfxContext.GetRenderStates().SetBlend (alpha, src, dest);

    // apply the bg colour
    gPainter.Paint2DQuadColor(GfxContext, bkg_box, _background_color);

    // apply the shine
    GfxContext.GetRenderStates().SetBlend(true, GL_DST_COLOR, GL_ONE);
    nux::TexCoordXForm texxform;
    texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
    texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
    texxform.uoffset = (1.0f / launcher_sheen_->GetWidth()); // TODO (gord) don't use absolute values here
    texxform.voffset = (1.0f / launcher_sheen_->GetHeight()) * panel_height;
    GfxContext.QRP_1Tex(base.x, base.y, base.width, base.height,
                        launcher_sheen_->GetDeviceTexture(),
                        texxform,
                        nux::color::White);

    //reset the blend state
    GfxContext.GetRenderStates().SetBlend (alpha, src, dest);
  }
  else
  {
    nux::Color color = _background_color;
    color.alpha = options()->background_alpha;
    gPainter.Paint2DQuadColor(GfxContext, bkg_box, color);
  }

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  icon_renderer->PreprocessIcons(args, base);
  EventLogic();

  /* draw launcher */
  for (rev_it = args.rbegin(); rev_it != args.rend(); rev_it++)
  {
    if ((*rev_it).stick_thingy)
      gPainter.Paint2DQuadColor(GfxContext,
                                nux::Geometry(bkg_box.x, (*rev_it).render_center.y - 3, bkg_box.width, 2),
                                nux::Color(0xAAAAAAAA));

    if ((*rev_it).skip)
      continue;

    icon_renderer->RenderIcon(GfxContext, *rev_it, bkg_box, base);
  }

  if (!_dash_is_open)
  {
    const double right_line_opacity = 0.15f * launcher_alpha;

    gPainter.Paint2DQuadColor(GfxContext,
                              nux::Geometry(bkg_box.x + bkg_box.width,
                                            bkg_box.y,
                                            RIGHT_LINE_WIDTH,
                                            bkg_box.height),
                              nux::color::White * right_line_opacity);

    gPainter.Paint2DQuadColor(GfxContext,
                              nux::Geometry(bkg_box.x,
                                            bkg_box.y,
                                            bkg_box.width,
                                            20),
                              nux::Color(0x60000000),
                              nux::Color(0x00000000),
                              nux::Color(0x00000000),
                              nux::Color(0x60000000));
  }

  // FIXME: can be removed for a bgk_box->SetAlpha once implemented
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::DST_IN);
  nux::Color alpha_mask = nux::Color(0xFFAAAAAA) * launcher_alpha;
  gPainter.Paint2DQuadColor(GfxContext, bkg_box, alpha_mask);

  GfxContext.GetRenderStates().SetColorMask(true, true, true, true);
  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  gPainter.PopBackground(push_count);
  GfxContext.PopClippingRectangle();
  GfxContext.PopClippingRectangle();
}

void Launcher::PostDraw(nux::GraphicsEngine& GfxContext, bool force_draw)
{
}

void Launcher::PreLayoutManagement()
{
  View::PreLayoutManagement();
  if (view_layout_)
  {
    view_layout_->SetGeometry(GetGeometry());
  }
}

long Launcher::PostLayoutManagement(long LayoutResult)
{
  View::PostLayoutManagement(LayoutResult);

  SetMousePosition(0, 0);

  return nux::eCompliantHeight | nux::eCompliantWidth;
}

void Launcher::PositionChildLayout(float offsetX, float offsetY)
{
}

void Launcher::OnDragWindowAnimCompleted()
{
  if (_drag_window)
    _drag_window->ShowWindow(false);

  EnsureAnimation();
}


gboolean Launcher::StartIconDragTimeout(gpointer data)
{
  Launcher* self = (Launcher*) data;

  // if we are still waiting…
  if (self->GetActionState() == ACTION_NONE)
  {
    if (self->_icon_under_mouse)
    {
      self->_icon_under_mouse->mouse_leave.emit(self->monitor);
      self->_icon_under_mouse = nullptr;
    }
    self->_initial_drag_animation = true;
    self->StartIconDragRequest(self->GetMouseX(), self->GetMouseY());
  }
  self->_start_dragicon_handle = 0;
  return false;
}

void Launcher::StartIconDragRequest(int x, int y)
{
  AbstractLauncherIcon* drag_icon = MouseIconIntersection((int)(GetGeometry().x / 2.0f), y);

  // FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
  // on an internal Launcher property then
  if (drag_icon && (_last_button_press == 1) && _model->IconHasSister(drag_icon))
  {
    SetActionState(ACTION_DRAG_ICON);
    StartIconDrag(drag_icon);
    UpdateDragWindowPosition(drag_icon->GetCenter(monitor).x, drag_icon->GetCenter(monitor).y);
    if (_initial_drag_animation)
    {
      _drag_window->SetAnimationTarget(x, y + _drag_window->GetGeometry().height / 2);
      _drag_window->StartAnimation();
    }
    EnsureAnimation();
  }
  else
  {
    _drag_icon = NULL;
    if (_drag_window)
    {
      _drag_window->ShowWindow(false);
      _drag_window->UnReference();
      _drag_window = NULL;
    }

  }
}

void Launcher::StartIconDrag(AbstractLauncherIcon* icon)
{
  if (!icon)
    return;

  _hide_machine->SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, true);
  _drag_icon = icon;

  if (_drag_window)
  {
    _drag_window->ShowWindow(false);
    _drag_window->UnReference();
    _drag_window = NULL;
  }

  _offscreen_drag_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(_icon_size, _icon_size, 1, nux::BITFMT_R8G8B8A8);
  _drag_window = new LauncherDragWindow(_offscreen_drag_texture);
  _drag_window->SinkReference();

  _render_drag_window = true;

  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_LAUNCHER_ICON_START_DND, NULL);
}

void Launcher::EndIconDrag()
{
  if (_drag_window)
  {
    AbstractLauncherIcon* hovered_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

    if (hovered_icon && hovered_icon->Type() == AbstractLauncherIcon::TYPE_TRASH)
    {
      hovered_icon->SetQuirk(AbstractLauncherIcon::QUIRK_PULSE_ONCE, true);

      launcher_removerequest.emit(_drag_icon);

      _drag_window->ShowWindow(false);
      EnsureAnimation();
    }
    else
    {
      _model->Save();

      _drag_window->SetAnimationTarget((int)(_drag_icon->GetCenter(monitor).x), (int)(_drag_icon->GetCenter(monitor).y));
      _drag_window->StartAnimation();

      if (_drag_window->on_anim_completed.connected())
        _drag_window->on_anim_completed.disconnect();
      _drag_window->on_anim_completed = _drag_window->anim_completed.connect(sigc::mem_fun(this, &Launcher::OnDragWindowAnimCompleted));
    }
  }

  if (MouseBeyondDragThreshold())
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);

  _render_drag_window = false;

  _hide_machine->SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, false);
  UBusServer* ubus = ubus_server_get_default();
  ubus_server_send_message(ubus, UBUS_LAUNCHER_ICON_END_DND, NULL);
}

void Launcher::UpdateDragWindowPosition(int x, int y)
{
  if (_drag_window)
  {
    nux::Geometry geo = _drag_window->GetGeometry();
    _drag_window->SetBaseXY(x - geo.width / 2 + _parent->GetGeometry().x, y - geo.height / 2 + _parent->GetGeometry().y);

    AbstractLauncherIcon* hovered_icon = MouseIconIntersection((int)(GetGeometry().x / 2.0f), y);

    struct timespec current;
    clock_gettime(CLOCK_MONOTONIC, &current);
    if (_drag_icon && hovered_icon && _drag_icon != hovered_icon)
    {
      float progress = DragThresholdProgress(current);

      if (progress >= 1.0f)
        _model->ReorderSmart(_drag_icon, hovered_icon, true);
      else if (progress == 0.0f)
        _model->ReorderBefore(_drag_icon, hovered_icon, false);
    }
  }
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  _last_button_press = nux::GetEventButton(button_flags);
  SetMousePosition(x, y);

  MouseDownLogic(x, y, button_flags, key_flags);
  EnsureAnimation();
}

void Launcher::RecvMouseUp(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);
  nux::Geometry geo = GetGeometry();

  MouseUpLogic(x, y, button_flags, key_flags);

  if (GetActionState() == ACTION_DRAG_ICON)
    EndIconDrag();

  if (GetActionState() == ACTION_DRAG_LAUNCHER)
    _hide_machine->SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, false);
  SetActionState(ACTION_NONE);
  _dnd_delta_x = 0;
  _dnd_delta_y = 0;
  _last_button_press = 0;
  EnsureAnimation();
}

void Launcher::RecvMouseDrag(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  /* FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
   * on an internal Launcher property then
   */

  if (_last_button_press != 1)
    return;

  SetMousePosition(x, y);

  // FIXME: hack (see SetupRenderArg)
  _initial_drag_animation = false;

  _dnd_delta_y += dy;
  _dnd_delta_x += dx;

  if (nux::Abs(_dnd_delta_y) < MOUSE_DEADZONE &&
      nux::Abs(_dnd_delta_x) < MOUSE_DEADZONE &&
      GetActionState() == ACTION_NONE)
    return;

  if (_icon_under_mouse)
  {
    _icon_under_mouse->mouse_leave.emit(monitor);
    _icon_under_mouse = nullptr;
  }

  if (GetActionState() == ACTION_NONE)
  {
    if (nux::Abs(_dnd_delta_y) >= nux::Abs(_dnd_delta_x))
    {
      _launcher_drag_delta += _dnd_delta_y;
      SetActionState(ACTION_DRAG_LAUNCHER);
      _hide_machine->SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, true);
    }
    else
    {
      StartIconDragRequest(x, y);
    }
  }
  else if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    _launcher_drag_delta += dy;
    ubus_server_send_message(ubus_server_get_default(),
                             UBUS_LAUNCHER_END_DND,
                             NULL);
  }
  else if (GetActionState() == ACTION_DRAG_ICON)
  {
    UpdateDragWindowPosition(x, y);
  }

  EnsureAnimation();
}

void Launcher::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);
  SetStateMouseOverLauncher(true);

  EventLogic();
  EnsureAnimation();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);
  SetStateMouseOverLauncher(false);
  //AbstractLauncherIcon::SetSkipTooltipDelay(false);

  EventLogic();
  EnsureAnimation();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);

  if (!_hidden)
  {
    _postreveal_mousemove_delta_x += dx;
    _postreveal_mousemove_delta_y += dy;

    // check the state before changing it to avoid uneeded hide calls
    if (!_hide_machine->GetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL) &&
        (nux::Abs(_postreveal_mousemove_delta_x) > MOUSE_DEADZONE ||
         nux::Abs(_postreveal_mousemove_delta_y) > MOUSE_DEADZONE))
      _hide_machine->SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
  }



  // Every time the mouse moves, we check if it is inside an icon...
  EventLogic();
}

void Launcher::RecvMouseWheel(int x, int y, int wheel_delta, unsigned long button_flags, unsigned long key_flags)
{
  if (!_hovered)
    return;

  if (wheel_delta < 0)
  {
    // scroll up
    _launcher_drag_delta -= 10;
  }
  else
  {
    // scroll down
    _launcher_drag_delta += 10;
  }

  EnsureAnimation();
}

void Launcher::OnPointerBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event)
{
  nux::Geometry abs_geo = GetAbsoluteGeometry();

  bool apply_to_reveal = false;
  if (_hidden && event->x >= abs_geo.x && event->x <= abs_geo.x + abs_geo.width)
  {
    if (options()->reveal_trigger == RevealTrigger::EDGE)
    {
      if (event->y >= abs_geo.y)
        apply_to_reveal = true;
    }
    else if (options()->reveal_trigger == RevealTrigger::CORNER)
    {
      if (event->y < abs_geo.y)
        apply_to_reveal = true;
    }
  }

  if (apply_to_reveal)
  {
    _hide_machine->AddRevealPressure(event->velocity);
    decaymulator_->value = 0;
  }
  else if (abs_geo.x > 0)
  {
    decaymulator_->value = decaymulator_->value + event->velocity;
    if (decaymulator_->value > _edge_overcome_pressure)
    {
      _pointer_barrier->ReleaseBarrier(event->event_id);
    }
  }
}

bool Launcher::IsInKeyNavMode() const
{
  return _hide_machine->GetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE);
}

void Launcher::EnterKeyNavMode()
{
  _hide_machine->SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, true);
  _hover_machine->SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, true);
}

void Launcher::ExitKeyNavMode()
{
  _hide_machine->SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, false);
  _hover_machine->SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, false);
}

void Launcher::RecvQuicklistOpened(QuicklistView* quicklist)
{
  UScreen* uscreen = UScreen::GetDefault();
  if (uscreen->GetMonitorGeometry(monitor).IsInside(nux::Point(quicklist->GetGeometry().x, quicklist->GetGeometry().y)))
  {
    _hide_machine->SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, true);
    _hover_machine->SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, true);
    EventLogic();
    EnsureAnimation();
  }
}

void Launcher::RecvQuicklistClosed(QuicklistView* quicklist)
{
  nux::Point pt = nux::GetWindowCompositor().GetMousePosition();
  if (!GetAbsoluteGeometry().IsInside(pt))
  {
    // The Quicklist just closed and the mouse is outside the launcher.
    SetHover(false);
    SetStateMouseOverLauncher(false);
  }
  // Cancel any prior state that was set before the Quicklist appeared.
  SetActionState(ACTION_NONE);

  _hide_machine->SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, false);
  _hover_machine->SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, false);

  EventLogic();
  EnsureAnimation();
}

void Launcher::EventLogic()
{
  if (GetActionState() == ACTION_DRAG_ICON ||
      GetActionState() == ACTION_DRAG_LAUNCHER)
    return;

  AbstractLauncherIcon* launcher_icon = nullptr;

  if (!_hidden && !IsInKeyNavMode() && _hovered)
  {
    launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);
  }


  if (_icon_under_mouse && (_icon_under_mouse != launcher_icon))
  {
    _icon_under_mouse->mouse_leave.emit(monitor);
    _icon_under_mouse = nullptr;
  }

  if (launcher_icon && (_icon_under_mouse != launcher_icon))
  {
    launcher_icon->mouse_enter.emit(monitor);
    _icon_under_mouse = launcher_icon;

    _hide_machine->SetQuirk(LauncherHideMachine::LAST_ACTION_ACTIVATE, false);
  }
}

void Launcher::MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon* launcher_icon = nullptr;
  launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  _hide_machine->SetQuirk(LauncherHideMachine::LAST_ACTION_ACTIVATE, false);

  if (launcher_icon)
  {
    _icon_mouse_down = launcher_icon;
    // if MouseUp after the time ended -> it's an icon drag, otherwise, it's starting an app
    if (_start_dragicon_handle > 0)
      g_source_remove(_start_dragicon_handle);
    _start_dragicon_handle = g_timeout_add(START_DRAGICON_DURATION, &Launcher::StartIconDragTimeout, this);

    launcher_icon->mouse_down.emit(nux::GetEventButton(button_flags), monitor);
  }
}

void Launcher::MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon* launcher_icon = nullptr;

  launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  if (_start_dragicon_handle > 0)
    g_source_remove(_start_dragicon_handle);
  _start_dragicon_handle = 0;

  if (_icon_mouse_down && (_icon_mouse_down == launcher_icon))
  {
    _icon_mouse_down->mouse_up.emit(nux::GetEventButton(button_flags), monitor);

    if (GetActionState() == ACTION_NONE)
    {
      _icon_mouse_down->mouse_click.emit(nux::GetEventButton(button_flags), monitor);
    }
  }

  if (launcher_icon && (_icon_mouse_down != launcher_icon))
  {
    launcher_icon->mouse_up.emit(nux::GetEventButton(button_flags), monitor);
  }

  if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_END]);
  }

  _icon_mouse_down = nullptr;
}

AbstractLauncherIcon* Launcher::MouseIconIntersection(int x, int y)
{
  LauncherModel::iterator it;
  // We are looking for the icon at screen coordinates x, y;
  nux::Point2 mouse_position(x, y);
  int inside = 0;

  for (it = _model->begin(); it != _model->end(); it++)
  {
    if (!(*it)->GetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE))
      continue;

    nux::Point2 screen_coord [4];
    for (int i = 0; i < 4; ++i)
    {
      auto hit_transform = (*it)->GetTransform(AbstractLauncherIcon::TRANSFORM_HIT_AREA, monitor);
      screen_coord [i].x = hit_transform [i].x;
      screen_coord [i].y = hit_transform [i].y;
    }
    inside = PointInside2DPolygon(screen_coord, 4, mouse_position, 1);
    if (inside)
      return (*it);
  }

  return 0;
}

void
Launcher::RenderIconToTexture(nux::GraphicsEngine& GfxContext, AbstractLauncherIcon* icon, nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{
  RenderArg arg;
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  SetupRenderArg(icon, current, arg);
  arg.render_center = nux::Point3(roundf(_icon_size / 2.0f), roundf(_icon_size / 2.0f), 0.0f);
  arg.logical_center = arg.render_center;
  arg.x_rotation = 0.0f;
  arg.running_arrow = false;
  arg.active_arrow = false;
  arg.skip = false;
  arg.window_indicators = 0;
  arg.alpha = 1.0f;

  std::list<RenderArg> drag_args;
  drag_args.push_front(arg);

  SetOffscreenRenderTarget(texture);
  icon_renderer->PreprocessIcons(drag_args, nux::Geometry(0, 0, _icon_size, _icon_size));
  icon_renderer->RenderIcon(nux::GetWindowThread()->GetGraphicsEngine(), arg, nux::Geometry(0, 0, _icon_size, _icon_size), nux::Geometry(0, 0, _icon_size, _icon_size));
  RestoreSystemRenderTarget();
}

void
Launcher::SetOffscreenRenderTarget(nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{
  int width = texture->GetWidth();
  int height = texture->GetHeight();

  auto graphics_display = nux::GetGraphicsDisplay();
  auto gpu_device = graphics_display->GetGpuDevice();
  gpu_device->FormatFrameBufferObject(width, height, nux::BITFMT_R8G8B8A8);
  gpu_device->SetColorRenderTargetSurface(0, texture->GetSurfaceLevel(0));
  gpu_device->ActivateFrameBuffer();

  auto graphics_engine = graphics_display->GetGraphicsEngine();
  graphics_engine->SetContext(0, 0, width, height);
  graphics_engine->SetViewport(0, 0, width, height);
  graphics_engine->Push2DWindow(width, height);
  graphics_engine->EmptyClippingRegion();
}

void
Launcher::RestoreSystemRenderTarget()
{
  nux::GetWindowCompositor().RestoreRenderingSurface();
}

void Launcher::OnDNDDataCollected(const std::list<char*>& mimes)
{
  _dnd_data.Reset();

  unity::glib::String uri_list_const(g_strdup("text/uri-list"));

  for (auto it : mimes)
  {
    if (!g_str_equal(it, uri_list_const.Value()))
      continue;

    _dnd_data.Fill(nux::GetWindowThread()->GetGraphicsDisplay().GetDndData(uri_list_const.Value()));
    break;
  }

  if (!_dnd_data.Uris().size())
    return;

  _hide_machine->SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, true);

  if (_dash_is_open)
    SaturateIcons();

  for (auto it : _dnd_data.Uris())
  {
    if (g_str_has_suffix(it.c_str(), ".desktop"))
    {
      _steal_drag = true;
      break;
    }
  }

  if (!_steal_drag)
  {
    for (auto it : *_model)
    {
      if (it->QueryAcceptDrop(_dnd_data) != nux::DNDACTION_NONE)
        it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_PRELIGHT, true);
      else
        it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_DIM, true);
    }
  }
}

void
Launcher::ProcessDndEnter()
{
  SetStateMouseOverLauncher(true);

  _dnd_data.Reset();
  _drag_action = nux::DNDACTION_NONE;
  _steal_drag = false;
  _data_checked = false;
  _drag_edge_touching = false;
  _dnd_hovered_icon = nullptr;
}

void
Launcher::DndReset()
{
  _dnd_data.Reset();

  for (auto it : *_model)
  {
    it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_PRELIGHT, false);
    it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_DIM, false);
  }

  DndHoveredIconReset();
}

void Launcher::DndHoveredIconReset()
{
  _drag_edge_touching = false;
  SetActionState(ACTION_NONE);

  if (_steal_drag && _dnd_hovered_icon)
  {
    _dnd_hovered_icon->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, false);
    _dnd_hovered_icon->remove.emit(_dnd_hovered_icon);
  }

  if (!_steal_drag && _dnd_hovered_icon)
    _dnd_hovered_icon->SendDndLeave();

  _steal_drag = false;
  _dnd_hovered_icon = nullptr;
}

void
Launcher::ProcessDndLeave()
{
  SetStateMouseOverLauncher(false);

  DndHoveredIconReset();
}

void
Launcher::ProcessDndMove(int x, int y, std::list<char*> mimes)
{
  nux::Area* parent = GetToplevel();
  unity::glib::String uri_list_const(g_strdup("text/uri-list"));

  if (!_data_checked)
  {
    _data_checked = true;
    _dnd_data.Reset();

    // get the data
    for (auto it : mimes)
    {
      if (!g_str_equal(it, uri_list_const.Value()))
        continue;

      _dnd_data.Fill(nux::GetWindowThread()->GetGraphicsDisplay().GetDndData(uri_list_const.Value()));
      break;
    }

    // see if the launcher wants this one
    for (auto it : _dnd_data.Uris())
    {
      if (g_str_has_suffix(it.c_str(), ".desktop"))
      {
        _steal_drag = true;
        break;
      }
    }

    // only set hover once we know our first x/y
    SetActionState(ACTION_DRAG_EXTERNAL);
    SetStateMouseOverLauncher(true);

    if (!_steal_drag)
    {
      for (auto it : *_model)
      {
        if (it->QueryAcceptDrop(_dnd_data) != nux::DNDACTION_NONE)
          it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_PRELIGHT, true);
        else
          it->SetQuirk(AbstractLauncherIcon::QUIRK_DROP_DIM, true);
      }
    }
  }

  SetMousePosition(x - parent->GetGeometry().x, y - parent->GetGeometry().y);

  if (!_dash_is_open && _mouse_position.x == 0 && _mouse_position.y <= (_parent->GetGeometry().height - _icon_size - 2 * _space_between_icons) && !_drag_edge_touching)
  {
    if (_dnd_hovered_icon)
        _dnd_hovered_icon->SendDndLeave();

    _drag_edge_touching = true;
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_EDGE_TOUCH], &_times[TIME_DRAG_EDGE_TOUCH], ANIM_DURATION * 3);
    EnsureAnimation();
  }
  else if (_mouse_position.x != 0 && _drag_edge_touching)
  {
    _drag_edge_touching = false;
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_EDGE_TOUCH], &_times[TIME_DRAG_EDGE_TOUCH], ANIM_DURATION * 3);
    EnsureAnimation();
  }

  EventLogic();
  AbstractLauncherIcon* hovered_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  bool hovered_icon_is_appropriate = false;
  if (hovered_icon)
  {
    if (hovered_icon->Type() == AbstractLauncherIcon::TYPE_TRASH)
      _steal_drag = false;

    if (hovered_icon->Type() == AbstractLauncherIcon::TYPE_APPLICATION || hovered_icon->Type() == AbstractLauncherIcon::TYPE_EXPO)
      hovered_icon_is_appropriate = true;
  }

  if (_steal_drag)
  {
    _drag_action = nux::DNDACTION_COPY;
    if (!_dnd_hovered_icon && hovered_icon_is_appropriate)
    {
      _dnd_hovered_icon = new SpacerLauncherIcon();
      _dnd_hovered_icon->SetSortPriority(G_MAXINT);
      _model->AddIcon(_dnd_hovered_icon);
      _model->ReorderBefore(_dnd_hovered_icon, hovered_icon, true);
    }
    else if (_dnd_hovered_icon)
    {
      if (hovered_icon)
      {
        if (hovered_icon_is_appropriate)
        {
          _model->ReorderSmart(_dnd_hovered_icon, hovered_icon, true);
        }
        else
        {
          _dnd_hovered_icon->SetQuirk(AbstractLauncherIcon::QUIRK_VISIBLE, false);
          _dnd_hovered_icon->remove.emit(_dnd_hovered_icon);
          _dnd_hovered_icon = nullptr;
        }
      }
    }
  }
  else
  {
    if (!_drag_edge_touching && hovered_icon != _dnd_hovered_icon)
    {
      if (hovered_icon)
      {
        hovered_icon->SendDndEnter();
        _drag_action = hovered_icon->QueryAcceptDrop(_dnd_data);
      }
      else
      {
        _drag_action = nux::DNDACTION_NONE;
      }

      if (_dnd_hovered_icon)
        _dnd_hovered_icon->SendDndLeave();

      _dnd_hovered_icon = hovered_icon;
    }
  }

  bool accept;
  if (_drag_action != nux::DNDACTION_NONE)
    accept = true;
  else
    accept = false;

  SendDndStatus(accept, _drag_action, nux::Geometry(x, y, 1, 1));
}

void
Launcher::ProcessDndDrop(int x, int y)
{
  if (_steal_drag)
  {
    for (auto it : _dnd_data.Uris())
    {
      if (g_str_has_suffix(it.c_str(), ".desktop"))
      {
        char* path = nullptr;

        if (g_str_has_prefix(it.c_str(), "application://"))
        {
          const char* tmp = it.c_str() + strlen("application://");
          unity::glib::String tmp2(g_strdup_printf("file:///usr/share/applications/%s", tmp));
          path = g_filename_from_uri(tmp2.Value(), NULL, NULL);
        }
        else if (g_str_has_prefix(it.c_str(), "file://"))
        {
          path = g_filename_from_uri(it.c_str(), NULL, NULL);
        }

        if (path)
        {
          launcher_addrequest.emit(path, _dnd_hovered_icon);
          g_free(path);
        }
      }
    }
  }
  else if (_dnd_hovered_icon && _drag_action != nux::DNDACTION_NONE)
  {
    _dnd_hovered_icon->AcceptDrop(_dnd_data);
  }

  if (_drag_action != nux::DNDACTION_NONE)
    SendDndFinished(true, _drag_action);
  else
    SendDndFinished(false, _drag_action);

  // reset our shiz
  DndReset();
}

/*
 * Returns the current selected icon if it is in keynavmode
 * It will return NULL if it is not on keynavmode
 */
AbstractLauncherIcon*
Launcher::GetSelectedMenuIcon() const
{
  if (!IsInKeyNavMode())
    return NULL;
  return _model->Selection();
}

/* dbus handlers */

void
Launcher::handle_dbus_method_call(GDBusConnection*       connection,
                                  const gchar*           sender,
                                  const gchar*           object_path,
                                  const gchar*           interface_name,
                                  const gchar*           method_name,
                                  GVariant*              parameters,
                                  GDBusMethodInvocation* invocation,
                                  gpointer               user_data)
{

  if (g_strcmp0(method_name, "AddLauncherItemFromPosition") == 0)
  {
    gchar*  icon;
    gchar*  title;
    gint32  icon_x;
    gint32  icon_y;
    gint32  icon_size;
    gchar*  desktop_file;
    gchar*  aptdaemon_task;

    g_variant_get(parameters, "(ssiiiss)", &title, &icon, &icon_x, &icon_y, &icon_size, &desktop_file, &aptdaemon_task, NULL);

    Launcher* self = (Launcher*)user_data;
    self->launcher_addrequest_special.emit(desktop_file, nullptr, aptdaemon_task, icon);

    g_dbus_method_invocation_return_value(invocation, nullptr);
    g_free(icon);
    g_free(title);
    g_free(desktop_file);
    g_free(aptdaemon_task);
  }

}

void
Launcher::OnBusAcquired(GDBusConnection* connection,
                        const gchar*     name,
                        gpointer         user_data)
{
  GDBusNodeInfo* introspection_data = g_dbus_node_info_new_for_xml(introspection_xml, NULL);
  guint registration_id;

  if (!introspection_data)
  {
    LOG_WARNING(logger) << "No introspection data loaded. Won't get dynamic launcher addition.";
    return;
  }



  registration_id = g_dbus_connection_register_object(connection,
                                                      S_DBUS_PATH,
                                                      introspection_data->interfaces[0],
                                                      &interface_vtable,
                                                      user_data,
                                                      NULL,
                                                      NULL);
  if (!registration_id)
  {
    LOG_WARNING(logger) << "Object registration failed. Won't get dynamic launcher addition.";
  }
}

void
Launcher::OnNameAcquired(GDBusConnection* connection,
                         const gchar*     name,
                         gpointer         user_data)
{
  LOG_DEBUG(logger) << "Acquired the name " << name << " on the session bus";
}

void
Launcher::OnNameLost(GDBusConnection* connection,
                     const gchar*     name,
                     gpointer         user_data)
{
  LOG_DEBUG(logger) << "Lost the name " << name << " on the session bus";
}

//
// Key navigation
//
bool
Launcher::InspectKeyEvent(unsigned int eventType,
                          unsigned int keysym,
                          const char* character)
{
  // The Launcher accepts all key inputs.
  return true;
}

} // namespace launcher
} // namespace unity
