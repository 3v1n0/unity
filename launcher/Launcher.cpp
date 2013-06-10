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

#include <functional>

#include <Nux/Nux.h>
#include <Nux/VScrollBar.h>
#include <Nux/HLayout.h>
#include <Nux/VLayout.h>
#include <NuxCore/Logger.h>

#include <NuxGraphics/NuxGraphics.h>
#include <NuxGraphics/GestureEvent.h>
#include <NuxGraphics/GpuDevice.h>
#include <NuxGraphics/GLTextureResourceManager.h>

#include <Nux/BaseWindow.h>
#include <Nux/WindowCompositor.h>

#include "Launcher.h"
#include "AbstractLauncherIcon.h"
#include "unity-shared/PanelStyle.h"
#include "SpacerLauncherIcon.h"
#include "LauncherModel.h"
#include "QuicklistManager.h"
#include "QuicklistView.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/TimeUtil.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/IconLoader.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"
#include "unity-shared/GraphicsUtils.h"


#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include <boost/algorithm/string.hpp>
#include <sigc++/sigc++.h>

namespace unity
{
using ui::RenderArg;
using ui::Decaymulator;

namespace launcher
{
DECLARE_LOGGER(logger, "unity.launcher");

const char* window_title = "unity-launcher";

namespace
{
const int URGENT_BLINKS = 3;
const int WIGGLE_CYCLES = 6;

const int MAX_STARTING_BLINKS = 5;
const int STARTING_BLINK_LAMBDA = 3;

const int PULSE_BLINK_LAMBDA = 2;

const float BACKLIGHT_STRENGTH = 0.9f;
const int ICON_PADDING = 6;
const int RIGHT_LINE_WIDTH = 1;

const int ANIM_DURATION_SHORT_SHORT = 100;
const int ANIM_DURATION = 200;
const int ANIM_DURATION_LONG = 350;
const int START_DRAGICON_DURATION = 250;

const int MOUSE_DEADZONE = 15;
const float DRAG_OUT_PIXELS = 300.0f;

const int SCROLL_AREA_HEIGHT = 24;
const int SCROLL_FPS = 30;

const int BASE_URGENT_WIGGLE_PERIOD = 60; // In seconds
const int MAX_URGENT_WIGGLE_DELTA = 960;  // In seconds
const int DOUBLE_TIME = 2;

const std::string START_DRAGICON_TIMEOUT = "start-dragicon-timeout";
const std::string SCROLL_TIMEOUT = "scroll-timeout";
const std::string ANIMATION_IDLE = "animation-idle";
const std::string URGENT_TIMEOUT = "urgent-timeout";
}


NUX_IMPLEMENT_OBJECT_TYPE(Launcher);

const int Launcher::Launcher::ANIM_DURATION_SHORT = 125;

Launcher::Launcher(MockableBaseWindow* parent,
                   NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
#ifdef USE_X11
  , display(nux::GetGraphicsDisplay()->GetX11Display())
#else
  , display(0)
#endif
  , monitor(0)
  , _parent(parent)
  , _active_quicklist(nullptr)
  , _hovered(false)
  , _hidden(false)
  , _shortcuts_shown(false)
  , _data_checked(false)
  , _steal_drag(false)
  , _drag_edge_touching(false)
  , _initial_drag_animation(false)
  , _dash_is_open(false)
  , _hud_is_open(false)
  , _folded_angle(1.0f)
  , _neg_folded_angle(-1.0f)
  , _folded_z_distance(10.0f)
  , _edge_overcome_pressure(0.0f)
  , _launcher_action_state(ACTION_NONE)
  , _space_between_icons(5)
  , _icon_image_size(48)
  , _icon_image_size_delta(6)
  , _icon_glow_size(62)
  , _icon_size(_icon_image_size + _icon_image_size_delta)
  , _dnd_delta_y(0)
  , _dnd_delta_x(0)
  , _postreveal_mousemove_delta_x(0)
  , _postreveal_mousemove_delta_y(0)
  , _launcher_drag_delta(0)
  , _launcher_drag_delta_max(0)
  , _launcher_drag_delta_min(0)
  , _enter_y(0)
  , _last_button_press(0)
  , _urgent_wiggle_time(0)
  , _urgent_acked(false)
  , _urgent_timer_running(false)
  , _urgent_ack_needed(false)
  , _drag_out_delta_x(0.0f)
  , _drag_gesture_ongoing(false)
  , _last_reveal_progress(0.0f)
  , _selection_atom(0)
  , icon_renderer(std::make_shared<ui::IconRenderer>())
{
  m_Layout = new nux::HLayout(NUX_TRACKER_LOCATION);
  icon_renderer->monitor = monitor();

  bg_effect_helper_.owner = this;
  bg_effect_helper_.enabled = false;

  SetCompositionLayout(m_Layout);
  CaptureMouseDownAnyWhereElse(true);
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptMouseWheelEvent(true);
  SetDndEnabled(false, true);

  _hide_machine.should_hide_changed.connect(sigc::mem_fun(this, &Launcher::SetHidden));
  _hide_machine.reveal_progress.changed.connect([&](float value) { EnsureAnimation(); });
  _hover_machine.should_hover_changed.connect(sigc::mem_fun(this, &Launcher::SetHover));

  mouse_down.connect(sigc::mem_fun(this, &Launcher::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &Launcher::RecvMouseUp));
  mouse_drag.connect(sigc::mem_fun(this, &Launcher::RecvMouseDrag));
  mouse_enter.connect(sigc::mem_fun(this, &Launcher::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Launcher::RecvMouseLeave));
  mouse_move.connect(sigc::mem_fun(this, &Launcher::RecvMouseMove));
  mouse_wheel.connect(sigc::mem_fun(this, &Launcher::RecvMouseWheel));
  //OnEndFocus.connect   (sigc::mem_fun (this, &Launcher::exitKeyNavMode));

  QuicklistManager& ql_manager = *(QuicklistManager::Default());
  ql_manager.quicklist_opened.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistOpened));
  ql_manager.quicklist_closed.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistClosed));

  WindowManager& wm = WindowManager::Default();
  wm.initiate_spread.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.initiate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.terminate_spread.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.terminate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &Launcher::EnsureAnimation));

  // 0 out timers to avoid wonky startups
  for (int i = 0; i < TIME_LAST; ++i)
  {
    _times[i].tv_sec = 0;
    _times[i].tv_nsec = 0;
  }

  _urgent_finished_time.tv_sec = 0;
  _urgent_finished_time.tv_nsec = 0;

  ubus_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &Launcher::OnOverlayShown));
  ubus_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &Launcher::OnOverlayHidden));
  ubus_.RegisterInterest(UBUS_LAUNCHER_LOCK_HIDE, sigc::mem_fun(this, &Launcher::OnLockHideChanged));

  icon_renderer->SetTargetSize(_icon_size, _icon_image_size, _space_between_icons);

  TextureCache& cache = TextureCache::GetDefault();
  launcher_sheen_ = cache.FindTexture("dash_sheen.png");
  launcher_pressure_effect_ = cache.FindTexture("launcher_pressure_effect.png");

  options.changed.connect(sigc::mem_fun(this, &Launcher::OnOptionsChanged));
  monitor.changed.connect(sigc::mem_fun(this, &Launcher::OnMonitorChanged));
}

/* Introspection */
std::string Launcher::GetName() const
{
  return "Launcher";
}

#ifdef NUX_GESTURES_SUPPORT
void Launcher::OnDragStart(const nux::GestureEvent &event)
{
  _drag_gesture_ongoing = true;
  if (_hidden)
  {
    _drag_out_delta_x = 0.0f;
  }
  else
  {
    _drag_out_delta_x = DRAG_OUT_PIXELS;
    _hide_machine.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
  }
}

void Launcher::OnDragUpdate(const nux::GestureEvent &event)
{
  _drag_out_delta_x =
    CLAMP(_drag_out_delta_x + event.GetDelta().x, 0.0f, DRAG_OUT_PIXELS);
  EnsureAnimation();
}

void Launcher::OnDragFinish(const nux::GestureEvent &event)
{
  if (_drag_out_delta_x >= DRAG_OUT_PIXELS - 90.0f)
    _hide_machine.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, true);
  TimeUtil::SetTimeStruct(&_times[TIME_DRAG_OUT],
                          &_times[TIME_DRAG_OUT],
                          ANIM_DURATION_SHORT);
  EnsureAnimation();
  _drag_gesture_ongoing = false;
}
#endif

void Launcher::AddProperties(GVariantBuilder* builder)
{
  timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  unity::variant::BuilderWrapper(builder)
  .add(GetAbsoluteGeometry())
  .add("hover-progress", GetHoverProgress(current))
  .add("dnd-exit-progress", DnDExitProgress(current))
  .add("autohide-progress", AutohideProgress(current))
  .add("dnd-delta", _dnd_delta_y)
  .add("hovered", _hovered)
  .add("hidemode", options()->hide_mode)
  .add("hidden", _hidden)
  .add("is_showing", ! _hidden)
  .add("monitor", monitor())
  .add("quicklist-open", _hide_machine.GetQuirk(LauncherHideMachine::QUICKLIST_OPEN))
  .add("hide-quirks", _hide_machine.DebugHideQuirks())
  .add("hover-quirks", _hover_machine.DebugHoverQuirks())
  .add("icon-size", _icon_size)
  .add("shortcuts_shown", _shortcuts_shown)
  .add("tooltip-shown", _active_tooltip != nullptr);
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
  _hide_machine.SetQuirk(LauncherHideMachine::MOUSE_OVER_LAUNCHER, over_launcher);
  _hide_machine.SetQuirk(LauncherHideMachine::REVEAL_PRESSURE_PASS, false);
  _hover_machine.SetQuirk(LauncherHoverMachine::MOUSE_OVER_LAUNCHER, over_launcher);
  tooltip_manager_.SetHover(over_launcher);
}

void Launcher::SetIconUnderMouse(AbstractLauncherIcon::Ptr const& icon)
{
  if (_icon_under_mouse == icon)
    return;

  if (_icon_under_mouse)
    _icon_under_mouse->mouse_leave.emit(monitor);
  if (icon)
    icon->mouse_enter.emit(monitor);

  _icon_under_mouse = icon;
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

  if (_drag_gesture_ongoing
      || _hide_machine.GetQuirk(LauncherHideMachine::MT_DRAG_OUT))
    return progress;
  else
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

void Launcher::EnsureAnimation()
{
  QueueDraw();
}

bool Launcher::IconNeedsAnimation(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::VISIBLE);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::RUNNING);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::STARTING);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::URGENT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * URGENT_BLINKS * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PULSE_ONCE);
  if (unity::TimeUtil::TimeDelta(&current, &time) < (ANIM_DURATION_LONG * PULSE_BLINK_LAMBDA * 2))
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PRESENTED);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::UNFOLDED);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::SHIMMER);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_LONG)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::CENTER_SAVED);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PROGRESS);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::DROP_DIM);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::DESAT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION_SHORT_SHORT)
    return true;

  time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::DROP_PRELIGHT);
  if (unity::TimeUtil::TimeDelta(&current, &time) < ANIM_DURATION)
    return true;

  return false;
}

bool Launcher::AnimationInProgress() const
{
  // performance here can be improved by caching the longer remaining animation found and short circuiting to that each time
  // this way extra checks may be avoided

  if (_last_reveal_progress != _hide_machine.reveal_progress)
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
  for (auto const &icon : *_model)
    if (IconNeedsAnimation(icon, current))
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


float Launcher::IconVisibleProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  if (!icon->IsVisibleOnMonitor(monitor))
    return 0.0f;

  if (icon->GetIconType() == AbstractLauncherIcon::IconType::HUD)
  {
    return icon->IsVisible() ? 1.0f : 0.0f;
  }

  if (icon->IsVisible())
  {
    struct timespec icon_visible_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::VISIBLE);
    DeltaTime enter_ms = unity::TimeUtil::TimeDelta(&current, &icon_visible_time);
    return CLAMP((float) enter_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  }
  else
  {
    struct timespec icon_hide_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::VISIBLE);
    DeltaTime hide_ms = unity::TimeUtil::TimeDelta(&current, &icon_hide_time);
    return 1.0f - CLAMP((float) hide_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);
  }
}

void Launcher::SetDndDelta(float x, float y, nux::Geometry const& geo, timespec const& current)
{
  AbstractLauncherIcon::Ptr const& anchor = MouseIconIntersection(x, _enter_y);

  if (anchor)
  {
    float position = y;
    for (AbstractLauncherIcon::Ptr const& model_icon : *_model)
    {
      if (model_icon == anchor)
      {
        position += _icon_size / 2;
        _launcher_drag_delta = _enter_y - position;

        if (position + _icon_size / 2 + _launcher_drag_delta > geo.height)
          _launcher_drag_delta -= (position + _icon_size / 2 + _launcher_drag_delta) - geo.height;

        break;
      }
      position += (_icon_size + _space_between_icons) * IconVisibleProgress(model_icon, current);
    }
  }
}

float Launcher::IconPresentProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec icon_present_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PRESENTED);
  DeltaTime ms = unity::TimeUtil::TimeDelta(&current, &icon_present_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::PRESENTED))
    return result;
  else
    return 1.0f - result;
}

float Launcher::IconUnfoldProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec icon_unfold_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::UNFOLDED);
  DeltaTime ms = unity::TimeUtil::TimeDelta(&current, &icon_unfold_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED))
    return result;
  else
    return 1.0f - result;
}

float Launcher::IconUrgentProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec urgent_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::URGENT);
  DeltaTime urgent_ms = unity::TimeUtil::TimeDelta(&current, &urgent_time);
  float result;

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
    result = CLAMP((float) urgent_ms / (float)(ANIM_DURATION_SHORT * WIGGLE_CYCLES), 0.0f, 1.0f);
  else
    result = CLAMP((float) urgent_ms / (float)(ANIM_DURATION_LONG * URGENT_BLINKS * 2), 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT))
    return result;
  else
    return 1.0f - result;
}

float Launcher::IconDropDimValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec dim_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::DROP_DIM);
  DeltaTime dim_ms = unity::TimeUtil::TimeDelta(&current, &dim_time);
  float result = CLAMP((float) dim_ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::DROP_DIM))
    return 1.0f - result;
  else
    return result;
}

float Launcher::IconDesatValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec dim_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::DESAT);
  DeltaTime ms = unity::TimeUtil::TimeDelta(&current, &dim_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION_SHORT_SHORT, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::DESAT))
    return 1.0f - result;
  else
    return result;
}

float Launcher::IconShimmerProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec shimmer_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::SHIMMER);
  DeltaTime shimmer_ms = unity::TimeUtil::TimeDelta(&current, &shimmer_time);
  return CLAMP((float) shimmer_ms / (float) ANIM_DURATION_LONG, 0.0f, 1.0f);
}

float Launcher::IconCenterTransitionProgress(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec save_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::CENTER_SAVED);
  DeltaTime save_ms = unity::TimeUtil::TimeDelta(&current, &save_time);
  return CLAMP((float) save_ms / (float) ANIM_DURATION, 0.0f, 1.0f);
}

float Launcher::IconUrgentPulseValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT))
    return 1.0f; // we are full on in a normal condition

  double urgent_progress = (double) IconUrgentProgress(icon, current);
  return 0.5f + (float)(std::cos(M_PI * (float)(URGENT_BLINKS * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconPulseOnceValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const &current) const
{
  struct timespec pulse_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PULSE_ONCE);
  DeltaTime pulse_ms = unity::TimeUtil::TimeDelta(&current, &pulse_time);
  double pulse_progress = (double) CLAMP((float) pulse_ms / (ANIM_DURATION_LONG * PULSE_BLINK_LAMBDA * 2), 0.0f, 1.0f);

  if (pulse_progress == 1.0f)
    icon->SetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE, false);

  return 0.5f + (float) (std::cos(M_PI * 2.0 * pulse_progress)) * 0.5f;
}

float Launcher::IconUrgentWiggleValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT))
    return 0.0f; // we are full on in a normal condition

  double urgent_progress = (double) IconUrgentProgress(icon, current);
  return 0.3f * (float)(std::sin(M_PI * (float)(WIGGLE_CYCLES * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconStartingBlinkValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
    return 1.0f;

  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::STARTING))
    return 1.0f;

  struct timespec starting_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::STARTING);
  DeltaTime starting_ms = unity::TimeUtil::TimeDelta(&current, &starting_time);
  double starting_progress = (double) CLAMP((float) starting_ms / (float)(ANIM_DURATION_LONG * STARTING_BLINK_LAMBDA), 0.0f, 1.0f);
  double val = IsBackLightModeToggles() ? 3.0f : 4.0f;
  return 1.0f-(0.5f + (float)(std::cos(M_PI * val * starting_progress)) * 0.5f);
}

float Launcher::IconStartingPulseValue(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
    return 1.0f;

  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::STARTING))
    return 1.0f;

  struct timespec starting_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::STARTING);
  DeltaTime starting_ms = unity::TimeUtil::TimeDelta(&current, &starting_time);
  double starting_progress = (double) CLAMP((float) starting_ms / (float)(ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2), 0.0f, 1.0f);

  if (starting_progress == 1.0f && !icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, false);
    icon->ResetQuirkTime(AbstractLauncherIcon::Quirk::STARTING);
  }

  return 1.0f-(0.5f + (float)(std::cos(M_PI * (float)(MAX_STARTING_BLINKS * 2) * starting_progress)) * 0.5f);
}

float Launcher::IconBackgroundIntensity(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  float result = 0.0f;

  struct timespec running_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::RUNNING);
  DeltaTime running_ms = unity::TimeUtil::TimeDelta(&current, &running_time);
  float running_progress = CLAMP((float) running_ms / (float) ANIM_DURATION_SHORT, 0.0f, 1.0f);

  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
    running_progress = 1.0f - running_progress;

  // After we finish a fade in from running, we can reset the quirk
  if (running_progress == 1.0f && icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
    icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, false);

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
      if (running_progress == 1.0f && icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
        icon->ResetQuirkTime(AbstractLauncherIcon::Quirk::STARTING);

      result = backlight_strength;
      if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
        result *= CLAMP(running_progress + IconStartingPulseValue(icon, current), 0.0f, 1.0f);
      else if (IsBackLightModeToggles())
        result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconStartingPulseValue(icon, current));
      else
        result = 1.0f - CLAMP(running_progress + IconStartingPulseValue(icon, current), 0.0f, 1.0f);
      break;
  }

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE))
  {
    if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
      result *= CLAMP(running_progress + IconPulseOnceValue(icon, current), 0.0f, 1.0f);
    else if (options()->backlight_mode() == BACKLIGHT_NORMAL)
      result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconPulseOnceValue(icon, current));
    else
      result = 1.0f - CLAMP(running_progress + IconPulseOnceValue(icon, current), 0.0f, 1.0f);
  }

  // urgent serves to bring the total down only
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT) && options()->urgent_animation() == URGENT_ANIMATION_PULSE)
    result *= 0.2f + 0.8f * IconUrgentPulseValue(icon, current);

  return result;
}

float Launcher::IconProgressBias(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current) const
{
  struct timespec icon_progress_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::PROGRESS);
  DeltaTime ms = unity::TimeUtil::TimeDelta(&current, &icon_progress_time);
  float result = CLAMP((float) ms / (float) ANIM_DURATION, 0.0f, 1.0f);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::PROGRESS))
    return -1.0f + result;
  else
    return result;
}

bool Launcher::IconDrawEdgeOnly(AbstractLauncherIcon::Ptr const& icon) const
{
  if (options()->backlight_mode() == BACKLIGHT_EDGE_TOGGLE)
    return true;

  if (options()->backlight_mode() == BACKLIGHT_NORMAL_EDGE_TOGGLE && !icon->WindowVisibleOnMonitor(monitor))
    return true;

  return false;
}

void Launcher::SetupRenderArg(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current, RenderArg& arg)
{
  float desat_value = IconDesatValue(icon, current);
  arg.icon                = icon.GetPointer();
  arg.alpha               = 0.2f + 0.8f * desat_value;
  arg.saturation          = desat_value;
  arg.colorify            = nux::color::White;
  arg.running_arrow       = icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING);
  arg.running_colored     = icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT);
  arg.draw_edge_only      = IconDrawEdgeOnly(icon);
  arg.active_colored      = false;
  arg.skip                = false;
  arg.stick_thingy        = false;
  arg.keyboard_nav_hl     = false;
  arg.progress_bias       = IconProgressBias(icon, current);
  arg.progress            = CLAMP(icon->GetProgress(), 0.0f, 1.0f);
  arg.draw_shortcut       = _shortcuts_shown && !_hide_machine.GetQuirk(LauncherHideMachine::PLACES_VISIBLE);
  arg.system_item         = icon->GetIconType() == AbstractLauncherIcon::IconType::HOME    ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::HUD;
  arg.colorify_background = icon->GetIconType() == AbstractLauncherIcon::IconType::HOME    ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::HUD     ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH   ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::DESKTOP ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::DEVICE  ||
                            icon->GetIconType() == AbstractLauncherIcon::IconType::EXPO;

  // trying to protect against flickering when icon is dragged from dash LP: #863230
  if (arg.alpha < 0.2)
  {
    arg.alpha = 0.2;
    arg.saturation = 0.0;
  }

  arg.active_arrow = icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE);

  /* BFB or HUD icons don't need the active arrow if the overaly is opened
   * in another monitor */
  if (arg.active_arrow && !IsOverlayOpen() &&
      (icon->GetIconType() == AbstractLauncherIcon::IconType::HOME ||
       icon->GetIconType() == AbstractLauncherIcon::IconType::HUD))
  {
    arg.active_arrow = false;
  }

  if (options()->show_for_all)
    arg.running_on_viewport = icon->WindowVisibleOnViewport();
  else
    arg.running_on_viewport = icon->WindowVisibleOnMonitor(monitor);

  guint64 shortcut = icon->GetShortcut();
  if (shortcut > 32)
    arg.shortcut_label = (char) shortcut;
  else
    arg.shortcut_label = 0;

  // we dont need to show strays
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING))
  {
    arg.window_indicators = 0;
  }
  else
  {
    if (options()->show_for_all)
      arg.window_indicators = std::max<int> (icon->WindowsOnViewport().size(), 1);
    else
      arg.window_indicators = std::max<int> (icon->WindowsForMonitor(monitor).size(), 1);

    if (icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH ||
        icon->GetIconType() == AbstractLauncherIcon::IconType::DEVICE)
    {
      // TODO: also these icons should respect the actual windows they have
      arg.window_indicators = 0;
    }
  }

  arg.backlight_intensity = IconBackgroundIntensity(icon, current);
  arg.shimmer_progress = IconShimmerProgress(icon, current);

  float urgent_progress = IconUrgentProgress(icon, current);

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT))
    urgent_progress = CLAMP(urgent_progress * 3.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle
  else
    urgent_progress = CLAMP(urgent_progress * 3.0f - 2.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle
  arg.glow_intensity = urgent_progress;

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT) && options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
  {
    arg.rotation.z = IconUrgentWiggleValue(icon, current);
  }

  if (IsInKeyNavMode())
  {
    if (icon == _model->Selection())
      arg.keyboard_nav_hl = true;
  }
}

void Launcher::FillRenderArg(AbstractLauncherIcon::Ptr const& icon,
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

  // trying to protect against flickering when icon is dragged from dash LP: #863230
  if (arg.alpha < 0.2)
  {
    arg.alpha = 0.2;
    arg.saturation = 0.0;
  }

  if (icon == _drag_icon)
  {
    if (MouseBeyondDragThreshold())
      arg.stick_thingy = true;

    if (GetActionState() == ACTION_DRAG_ICON ||
        (_drag_window && _drag_window->Animating()) ||
        icon->GetIconType() == AbstractLauncherIcon::IconType::SPACER)
    {
      arg.skip = true;
    }

    size_modifier *= DragThresholdProgress(current);
  }

  if (size_modifier <= 0.0f)
    arg.skip = true;

  // goes for 0.0f when fully unfolded, to 1.0f folded
  float folding_progress = CLAMP((center.y + _icon_size - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
  float unfold_progress = IconUnfoldProgress(icon, current);

  folding_progress *= 1.0f - unfold_progress;

  float half_size = (folded_size / 2.0f) + (_icon_size / 2.0f - folded_size / 2.0f) * (1.0f - folding_progress);
  float icon_hide_offset = autohide_offset;

  float present_progress = IconPresentProgress(icon, current);
  icon_hide_offset *= 1.0f - (present_progress * icon->PresentUrgency());

  // icon is crossing threshold, start folding
  center.z += folded_z_distance * folding_progress;
  arg.rotation.x = animation_neg_rads * folding_progress;

  float spacing_overlap = CLAMP((float)(center.y + (2.0f * half_size * size_modifier) + (_space_between_icons * size_modifier) - folding_threshold) / (float) _icon_size, 0.0f, 1.0f);
  float spacing = (_space_between_icons * (1.0f - spacing_overlap) + folded_spacing * spacing_overlap) * size_modifier;

  nux::Point3 centerOffset;
  float center_transit_progress = IconCenterTransitionProgress(icon, current);
  if (center_transit_progress <= 1.0f)
  {
    int saved_center = icon->GetSavedCenter(monitor).y - parent_abs_geo.y;
    centerOffset.y = (saved_center - (center.y + (half_size * size_modifier))) * (1.0f - center_transit_progress);
  }

  center.y += half_size * size_modifier;   // move to center

  arg.render_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y + centerOffset.y), roundf(center.z));
  arg.logical_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y), roundf(center.z));

  icon->SetCenter(nux::Point3(roundf(center.x), roundf(center.y), roundf(center.z)), monitor, parent_abs_geo);

  // FIXME: this is a hack, to avoid that we set the target to the end of the icon
  if (!_initial_drag_animation && icon == _drag_icon && _drag_window && _drag_window->Animating())
  {
    auto const& icon_center = _drag_icon->GetCenter(monitor);
    _drag_window->SetAnimationTarget(icon_center.x, icon_center.y);
  }

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

  if (max > 0.0f)
    color = color * (1.0f / max);

  return color;
}

void Launcher::RenderArgs(std::list<RenderArg> &launcher_args,
                          nux::Geometry& box_geo, float* launcher_alpha, nux::Geometry const& parent_abs_geo)
{
  nux::Geometry const& geo = GetGeometry();
  LauncherModel::iterator it;
  nux::Point3 center;
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  nux::Color const& colorify = FullySaturateColor(options()->background_color);

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
  for (it = _model->begin(); it != _model->end(); ++it)
  {
    float height = (_icon_size + _space_between_icons) * IconVisibleProgress(*it, current);
    sum += height;

    // magic constant must some day be explained, for now suffice to say this constant prevents the bottom from "marching";
    float magic_constant = 1.3f;

    float unfold_progress = IconUnfoldProgress(*it, current);
    folding_threshold -= CLAMP(sum - launcher_height, 0.0f, height * magic_constant) * (folding_constant + (1.0f - folding_constant) * unfold_progress);
  }

  if (sum - _space_between_icons <= launcher_height)
    folding_threshold = launcher_height;

  float autohide_offset = 0.0f;
  *launcher_alpha = 1.0f;
  if (options()->hide_mode != LAUNCHER_HIDE_NEVER || _hide_machine.GetQuirk(LauncherHideMachine::LOCK_HIDE))
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
  if (options()->hide_mode != LAUNCHER_HIDE_NEVER && drag_hide_progress > 0.0f)
  {
    autohide_offset -= geo.width * 0.25f * drag_hide_progress;

    if (drag_hide_progress >= 1.0f)
      _hide_machine.SetQuirk(LauncherHideMachine::DND_PUSHED_OFF, true);
  }

  // Inform the painter where to paint the box
  box_geo = geo;

  if (options()->hide_mode != LAUNCHER_HIDE_NEVER || _hide_machine.GetQuirk(LauncherHideMachine::LOCK_HIDE))
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

  
  // logically dnd exit only restores to the clamped ranges
  // hover_progress restores to 0
  _launcher_drag_delta_max = 0.0f;
  _launcher_drag_delta_min = MIN(0.0f, launcher_height - sum);

  if (hover_progress > 0.0f && _launcher_drag_delta != 0)
  {
    float delta_y = _launcher_drag_delta;

    if (_launcher_drag_delta > _launcher_drag_delta_max)
      delta_y = _launcher_drag_delta_max + DragLimiter(delta_y - _launcher_drag_delta_max);
    else if (_launcher_drag_delta < _launcher_drag_delta_min)
      delta_y = _launcher_drag_delta_min + DragLimiter(delta_y - _launcher_drag_delta_min);

    if (GetActionState() != ACTION_DRAG_LAUNCHER)
    {
      float dnd_progress = DnDExitProgress(current);

      if (_launcher_drag_delta > _launcher_drag_delta_max)
        delta_y = _launcher_drag_delta_max + (delta_y - _launcher_drag_delta_max) * dnd_progress;
      else if (_launcher_drag_delta < _launcher_drag_delta_min)
        delta_y = _launcher_drag_delta_min + (delta_y - _launcher_drag_delta_min) * dnd_progress;

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
  for (it = _model->main_begin(); it != _model->main_end(); ++it)
  {
    RenderArg arg;
    AbstractLauncherIcon::Ptr const& icon = *it;

    if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT) && options()->hide_mode == LAUNCHER_HIDE_AUTOHIDE)
    {
      HandleUrgentIcon(icon, current);
    }
    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads, current);
    arg.colorify = colorify;
    launcher_args.push_back(arg);
  }

  // compute maximum height of shelf
  float shelf_sum = 0.0f;
  for (it = _model->shelf_begin(); it != _model->shelf_end(); ++it)
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

  for (it = _model->shelf_begin(); it != _model->shelf_end(); ++it)
  {
    RenderArg arg;
    AbstractLauncherIcon::Ptr const& icon = *it;

    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads, current);
    arg.colorify = colorify;
    launcher_args.push_back(arg);
  }
}

/* End Render Layout Logic */

void Launcher::ForceReveal(bool force_reveal)
{
  _hide_machine.SetQuirk(LauncherHideMachine::TRIGGER_BUTTON_SHOW, force_reveal);
}

void Launcher::ShowShortcuts(bool show)
{
  _shortcuts_shown = show;
  _hide_machine.SetQuirk(LauncherHideMachine::SHORTCUT_KEYS_VISIBLE, show);
  EnsureAnimation();
}

void Launcher::OnLockHideChanged(GVariant *data)
{
  gboolean enable_lock = FALSE;
  g_variant_get(data, "(b)", &enable_lock);

  if (enable_lock)
  {
    _hide_machine.SetQuirk(LauncherHideMachine::LOCK_HIDE, true);
  }
  else
  {
    _hide_machine.SetQuirk(LauncherHideMachine::LOCK_HIDE, false);
  }
}

void Launcher::DesaturateIcons()
{
  for (auto icon : *_model)
  {
    if (icon->GetIconType () != AbstractLauncherIcon::IconType::HOME &&
        icon->GetIconType () != AbstractLauncherIcon::IconType::HUD)
    {
      icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true);
    }

    icon->HideTooltip();
  }
}

void Launcher::SaturateIcons()
{
  for (auto icon : *_model)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false);
  }
}

void Launcher::OnOverlayShown(GVariant* data)
{
  // check the type of overlay
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);
  std::string identity(overlay_identity.Str());

  LOG_DEBUG(logger) << "Overlay shown: " << identity
                    << ", " << (can_maximise ? "can maximise" : "can't maximise")
                    << ", on monitor " << overlay_monitor
                    << " (for monitor " << monitor() << ")";

  if (overlay_monitor == monitor())
  {
    if (identity == "dash")
    {
      _dash_is_open = true;
      _hide_machine.SetQuirk(LauncherHideMachine::PLACES_VISIBLE, true);
      _hover_machine.SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, true);
    }
    if (identity == "hud")
    {
      _hud_is_open = true;
    }
    bg_effect_helper_.enabled = true;
    // Don't desaturate icons if the mouse is over the launcher:
    if (!_hovered)
    {
      LOG_DEBUG(logger) << "Desaturate on monitor " << monitor();
      DesaturateIcons();
    }

    if (_icon_under_mouse)
      _icon_under_mouse->HideTooltip();
  }
  EnsureAnimation();
}

void Launcher::OnOverlayHidden(GVariant* data)
{
  // check the type of overlay
  unity::glib::String overlay_identity;
  gboolean can_maximise = FALSE;
  gint32 overlay_monitor = 0;
  int width, height;
  g_variant_get(data, UBUS_OVERLAY_FORMAT_STRING,
                &overlay_identity, &can_maximise, &overlay_monitor, &width, &height);

  std::string identity = overlay_identity.Str();

  LOG_DEBUG(logger) << "Overlay hidden: " << identity
                    << ", " << (can_maximise ? "can maximise" : "can't maximise")
                    << ", on monitor " << overlay_monitor
                    << " (for monitor" << monitor() << ")";

  if (overlay_monitor == monitor())
  {
    if (identity == "dash")
    {
      _hide_machine.SetQuirk(LauncherHideMachine::PLACES_VISIBLE, false);
      _hover_machine.SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, false);
      _dash_is_open = false;
    }
    else if (identity == "hud")
    {
      _hud_is_open = false;
    }

    // If they are both now shut, then disable the effect helper and saturate the icons.
    if (!IsOverlayOpen())
    {
      bg_effect_helper_.enabled = false;
      LOG_DEBUG(logger) << "Saturate on monitor " << monitor();
      SaturateIcons();
    }
  }
  EnsureAnimation();

  // as the leave event is no more received when the place is opened
  // FIXME: remove when we change the mouse grab strategy in nux
  nux::Point pt = nux::GetWindowCompositor().GetMousePosition();
  SetStateMouseOverLauncher(GetAbsoluteGeometry().IsInside(pt));
}

bool Launcher::IsOverlayOpen() const
{
  return _dash_is_open || _hud_is_open;
}

void Launcher::SetHidden(bool hide_launcher)
{
  if (hide_launcher == _hidden)
    return;

  _hidden = hide_launcher;
  _hide_machine.SetQuirk(LauncherHideMachine::LAUNCHER_HIDDEN, hide_launcher);
  _hover_machine.SetQuirk(LauncherHoverMachine::LAUNCHER_HIDDEN, hide_launcher);

  if (hide_launcher)
  {
    _hide_machine.SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, false);
    _hide_machine.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
    SetStateMouseOverLauncher(false);
  }

  _postreveal_mousemove_delta_x = 0;
  _postreveal_mousemove_delta_y = 0;

  TimeUtil::SetTimeStruct(&_times[TIME_AUTOHIDE], &_times[TIME_AUTOHIDE], ANIM_DURATION_SHORT);

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    _parent->EnableInputWindow(!hide_launcher, launcher::window_title, false, false);

  if (!hide_launcher && GetActionState() == ACTION_DRAG_EXTERNAL)
    DndReset();

  EnsureAnimation();

  hidden_changed.emit();
}

void Launcher::UpdateChangeInMousePosition(int delta_x, int delta_y)
{
  _postreveal_mousemove_delta_x += delta_x;
  _postreveal_mousemove_delta_y += delta_y;

  // check the state before changing it to avoid uneeded hide calls
  if (!_hide_machine.GetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL) &&
     (std::abs(_postreveal_mousemove_delta_x) > MOUSE_DEADZONE ||
      std::abs(_postreveal_mousemove_delta_y) > MOUSE_DEADZONE))
  {
    _hide_machine.SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
  }
}

int Launcher::GetMouseX() const
{
  return _mouse_position.x;
}

int Launcher::GetMouseY() const
{
  return _mouse_position.y;
}

void Launcher::OnPluginStateChanged()
{
  WindowManager& wm = WindowManager::Default();
  _hide_machine.SetQuirk(LauncherHideMachine::EXPO_ACTIVE, wm.IsExpoActive());
  _hide_machine.SetQuirk(LauncherHideMachine::SCALE_ACTIVE, wm.IsScaleActive());
}

LauncherHideMode Launcher::GetHideMode() const
{
  return options()->hide_mode;
}

/* End Launcher Show/Hide logic */

void Launcher::OnOptionsChanged(Options::Ptr options)
{
   UpdateOptions(options);
   options->option_changed.connect(sigc::mem_fun(this, &Launcher::OnOptionChanged));
}

void Launcher::OnOptionChanged()
{
  UpdateOptions(options());
}

void Launcher::OnMonitorChanged(int new_monitor)
{
  UScreen* uscreen = UScreen::GetDefault();
  auto monitor_geo = uscreen->GetMonitorGeometry(new_monitor);
  unity::panel::Style &panel_style = panel::Style::Instance();
  Resize(nux::Point(monitor_geo.x, monitor_geo.y + panel_style.panel_height),
         monitor_geo.height - panel_style.panel_height);
  icon_renderer->monitor = new_monitor;
}

void Launcher::UpdateOptions(Options::Ptr options)
{
  SetIconSize(options->tile_size, options->icon_size);
  SetHideMode(options->hide_mode);

  ConfigureBarrier();
  EnsureAnimation();
}

void Launcher::ConfigureBarrier()
{
  float decay_responsiveness_mult = ((options()->edge_responsiveness() - 1) * .3f) + 1;
  float reveal_responsiveness_mult = ((options()->edge_responsiveness() - 1) * .025f) + 1;

  _hide_machine.reveal_pressure = options()->edge_reveal_pressure() * reveal_responsiveness_mult;
  _hide_machine.edge_decay_rate = options()->edge_decay_rate() * decay_responsiveness_mult;
}

void Launcher::SetHideMode(LauncherHideMode hidemode)
{
  bool fixed_launcher = (hidemode == LAUNCHER_HIDE_NEVER);
  _parent->InputWindowEnableStruts(fixed_launcher);
  _hide_machine.SetMode(static_cast<LauncherHideMachine::HideMode>(hidemode));
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

nux::ObjectPtr<nux::View> const& Launcher::GetActiveTooltip() const
{
  return _active_tooltip;
}

nux::ObjectPtr<LauncherDragWindow> const& Launcher::GetDraggedIcon() const
{
  return _drag_window;
}

void Launcher::SetActionState(LauncherActionState actionstate)
{
  if (_launcher_action_state == actionstate)
    return;

  _launcher_action_state = actionstate;

  _hover_machine.SetQuirk(LauncherHoverMachine::LAUNCHER_IN_ACTION, (actionstate != ACTION_NONE));
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

  if (IsOverlayOpen() && !_hide_machine.GetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE))
  {
    if (hovered && !_hide_machine.GetQuirk(LauncherHideMachine::SHORTCUT_KEYS_VISIBLE))
      SaturateIcons();
    else
      DesaturateIcons();
  }

  EnsureAnimation();
}

bool Launcher::MouseOverTopScrollArea()
{
  return _mouse_position.y < SCROLL_AREA_HEIGHT;
}

bool Launcher::MouseOverBottomScrollArea()
{
  return _mouse_position.y >= GetGeometry().height - SCROLL_AREA_HEIGHT;
}

bool Launcher::OnScrollTimeout()
{
  bool continue_animation = true;
  int speed = 0;

  if (IsInKeyNavMode() || !_hovered ||
      GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    continue_animation = false;
  }
  else if (MouseOverTopScrollArea())
  {
    if (_launcher_drag_delta >= _launcher_drag_delta_max)
      continue_animation = false;
    else
    {
        speed = (SCROLL_AREA_HEIGHT - _mouse_position.y) / SCROLL_AREA_HEIGHT * SCROLL_FPS;
        _launcher_drag_delta += speed;
    }
  }
  else if (MouseOverBottomScrollArea())
  {
    if (_launcher_drag_delta <= _launcher_drag_delta_min)
      continue_animation = false;
    else
    {
        speed = ((_mouse_position.y + 1) - (GetGeometry().height - SCROLL_AREA_HEIGHT)) / SCROLL_AREA_HEIGHT * SCROLL_FPS;
        _launcher_drag_delta -= speed;
    } 
  }
  else
  {
    continue_animation = false;
  }

  if (continue_animation)
  {
    EnsureAnimation();
  }

  return continue_animation;
}

void Launcher::EnsureScrollTimer()
{
  bool needed = MouseOverTopScrollArea() || MouseOverBottomScrollArea();

  if (needed && !sources_.GetSource(SCROLL_TIMEOUT))
  {
    sources_.AddTimeout(20, sigc::mem_fun(this, &Launcher::OnScrollTimeout), SCROLL_TIMEOUT);
  }
  else if (!needed)
  {
    sources_.Remove(SCROLL_TIMEOUT);
  }
}

void Launcher::SetUrgentTimer(int urgent_wiggle_period)
{
  sources_.AddTimeoutSeconds(urgent_wiggle_period, sigc::mem_fun(this, &Launcher::OnUrgentTimeout), URGENT_TIMEOUT);
}

void Launcher::WiggleUrgentIcon(AbstractLauncherIcon::Ptr const& icon)
{
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false);
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true);

  clock_gettime(CLOCK_MONOTONIC, &_urgent_finished_time);
}

void Launcher::HandleUrgentIcon(AbstractLauncherIcon::Ptr const& icon, struct timespec const& current)
{
  struct timespec urgent_time = icon->GetQuirkTime(AbstractLauncherIcon::Quirk::URGENT);
  DeltaTime urgent_delta = unity::TimeUtil::TimeDelta(&urgent_time, &_urgent_finished_time);

  // If the Launcher is hidden, then add a timer to wiggle the urgent icons at 
  // certain intervals (1m, 2m, 4m, 8m, 16m, & 32m).
  if (_hidden && !_urgent_timer_running && urgent_delta > 0)
  {
    _urgent_timer_running = true;
    _urgent_ack_needed = true;
    SetUrgentTimer(BASE_URGENT_WIGGLE_PERIOD);
  }
  // If the Launcher is hidden, the timer is running, an urgent icon is newer than the last time
  // icons were wiggled, and the timer did not just start, then reset the timer since a new
  // urgent icon just showed up.
  else if (_hidden && _urgent_timer_running && urgent_delta > 0 && _urgent_wiggle_time != 0)
  {
    _urgent_wiggle_time = 0;
    SetUrgentTimer(BASE_URGENT_WIGGLE_PERIOD);
  }
  // If the Launcher is no longer hidden, then after the Launcher is fully revealed, wiggle the 
  // urgent icon and then stop the timer (if it's running).
  else if (!_hidden && _urgent_ack_needed)
  {
    if (_last_reveal_progress > 0)
    {
      _urgent_acked = false;
    }
    else
    {
      if (!_urgent_acked && IconUrgentProgress(icon, current) == 1.0f)
      {
        WiggleUrgentIcon(icon);
      }
      else if (IconUrgentProgress(icon, current) < 1.0f)
      {
        if (_urgent_timer_running)
        {
          sources_.Remove(URGENT_TIMEOUT);
          _urgent_timer_running = false;
        }
        _urgent_acked = true;
        _urgent_ack_needed = false;
      }
    }
  }
}

bool Launcher::OnUrgentTimeout()
{
  bool foundUrgent = false,
       continue_urgent = true;

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE && _hidden)
  {
    // Look for any icons that are still urgent and wiggle them
    for (auto icon : *_model)
    {
      if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT))
      {
        WiggleUrgentIcon(icon);

        foundUrgent = true;
      }
    }
  }
  // Update the time for when the next wiggle will occur.
  if (_urgent_wiggle_time == 0)
  {
    _urgent_wiggle_time = BASE_URGENT_WIGGLE_PERIOD;
  }
  else
  {
    _urgent_wiggle_time = _urgent_wiggle_time * DOUBLE_TIME;
  }

  // If no urgent icons were found or we have reached the time threshold,
  // then let's stop the timer.  Otherwise, start a new timer with the
  // updated wiggle time.
  if (!foundUrgent || (_urgent_wiggle_time > MAX_URGENT_WIGGLE_DELTA))
  {
    continue_urgent = false;
    _urgent_timer_running = false;
  }
  else
  {
    SetUrgentTimer(_urgent_wiggle_time);
  }

  return continue_urgent;
}

void Launcher::SetIconSize(int tile_size, int icon_size)
{
  ui::IconRenderer::DestroyShortcutTextures();

  _icon_size = tile_size;
  _icon_image_size = icon_size;
  _icon_image_size_delta = tile_size - icon_size;
  _icon_glow_size = icon_size + 14;

  icon_renderer->SetTargetSize(_icon_size, _icon_image_size, _space_between_icons);

  nux::Geometry const& parent_geo = _parent->GetGeometry();
  Resize(nux::Point(parent_geo.x, parent_geo.y), parent_geo.height);
}

int Launcher::GetIconSize() const
{
  return _icon_size;
}

void Launcher::Resize(nux::Point const& offset, int height)
{
  unsigned width = _icon_size + ICON_PADDING * 2 + RIGHT_LINE_WIDTH - 2;
  SetMaximumHeight(height);
  SetGeometry(nux::Geometry(0, 0, width, height));
  _parent->SetGeometry(nux::Geometry(offset.x, offset.y, width, height));

  ConfigureBarrier();
}

void Launcher::OnIconAdded(AbstractLauncherIcon::Ptr const& icon)
{
  EnsureAnimation();

  icon->needs_redraw.connect(sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));
  icon->tooltip_visible.connect(sigc::mem_fun(this, &Launcher::OnTooltipVisible));
}

void Launcher::OnIconRemoved(AbstractLauncherIcon::Ptr const& icon)
{
  if (icon->needs_redraw_connection.connected())
    icon->needs_redraw_connection.disconnect();

  SetIconUnderMouse(AbstractLauncherIcon::Ptr());
  if (icon == _icon_mouse_down)
    _icon_mouse_down = nullptr;
  if (icon == _drag_icon)
    _drag_icon = nullptr;

  EnsureAnimation();
}

void Launcher::OnOrderChanged()
{
  EnsureAnimation();
}

void Launcher::SetModel(LauncherModel::Ptr model)
{
  _model = model;

  for (auto icon : *_model)
    icon->needs_redraw.connect(sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));

  _model->icon_added.connect(sigc::mem_fun(this, &Launcher::OnIconAdded));
  _model->icon_removed.connect(sigc::mem_fun(this, &Launcher::OnIconRemoved));
  _model->order_changed.connect(sigc::mem_fun(this, &Launcher::OnOrderChanged));
  _model->selection_changed.connect(sigc::mem_fun(this, &Launcher::OnSelectionChanged));
}

LauncherModel::Ptr Launcher::GetModel() const
{
  return _model;
}

void Launcher::EnsureIconOnScreen(AbstractLauncherIcon::Ptr const& selection)
{
  nux::Geometry const& geo = GetGeometry();

  int natural_y = 0;
  for (auto icon : *_model)
  {
    if (!icon->IsVisible() || !icon->IsVisibleOnMonitor(monitor))
      continue;

    if (icon == selection)
      break;

    natural_y += _icon_size + _space_between_icons;
  }

  int max_drag_delta = geo.height - (natural_y + _icon_size + (2 * _space_between_icons));
  int min_drag_delta = -natural_y;

  _launcher_drag_delta = std::max<int>(min_drag_delta, std::min<int>(max_drag_delta, _launcher_drag_delta));
}

void Launcher::OnSelectionChanged(AbstractLauncherIcon::Ptr const& selection)
{
  if (IsInKeyNavMode())
  {
    EnsureIconOnScreen(selection);
    EnsureAnimation();
  }
}

void Launcher::OnIconNeedsRedraw(AbstractLauncherIcon::Ptr const& icon)
{
  EnsureAnimation();
}

void Launcher::OnTooltipVisible(nux::ObjectPtr<nux::View> view)
{
  _active_tooltip = view;
}

void Launcher::Draw(nux::GraphicsEngine& GfxContext, bool force_draw)
{

}

void Launcher::DrawContent(nux::GraphicsEngine& GfxContext, bool force_draw)
{
  nux::Geometry const& base = GetGeometry();
  nux::Geometry bkg_box;
  std::list<RenderArg> args;
  std::list<RenderArg>::reverse_iterator rev_it;
  float launcher_alpha = 1.0f;

  // rely on the compiz event loop to come back to us in a nice throttling
  if (AnimationInProgress())
  {
    auto idle = std::make_shared<glib::Idle>(glib::Source::Priority::DEFAULT);
    sources_.Add(idle, ANIMATION_IDLE);
    idle->Run([&]() {
      EnsureAnimation();
      return false;
    });
  }

  nux::ROPConfig ROP;
  ROP.Blend = false;
  ROP.SrcBlend = GL_ONE;
  ROP.DstBlend = GL_ONE_MINUS_SRC_ALPHA;

  nux::Geometry const& geo_absolute = GetAbsoluteGeometry();
  RenderArgs(args, bkg_box, &launcher_alpha, geo_absolute);
  bkg_box.width -= RIGHT_LINE_WIDTH;
  
  nux::Color clear_colour = nux::Color(0x00000000);
  
  if (Settings::Instance().GetLowGfxMode())
  {
    clear_colour = options()->background_color;
    clear_colour.alpha = 1.0f;
  }

  // clear region
  GfxContext.PushClippingRectangle(base);
  gPainter.PushDrawColorLayer(GfxContext, base, clear_colour, true, ROP);

  if (Settings::Instance().GetLowGfxMode() == false)
  {
    GfxContext.GetRenderStates().SetBlend(true);
    GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);
    GfxContext.GetRenderStates().SetColorMask(true, true, true, true);
  }

  int push_count = 1;

  // clip vertically but not horizontally
  GfxContext.PushClippingRectangle(nux::Geometry(base.x, bkg_box.y, base.width, bkg_box.height));

  float reveal_progress = _hide_machine.reveal_progress;
  if ((reveal_progress > 0 || _last_reveal_progress > 0) && launcher_pressure_effect_.IsValid())
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
    nux::Color pressure_color = nux::color::White * _last_reveal_progress;
    nux::TexCoordXForm texxform_pressure;
    GfxContext.QRP_1Tex(base.x, base.y, launcher_pressure_effect_->GetWidth(), base.height,
                        launcher_pressure_effect_->GetDeviceTexture(),
                        texxform_pressure,
                        pressure_color);
  }

  if (Settings::Instance().GetLowGfxMode() == false)
  {
    if (IsOverlayOpen())
    {
      nux::Geometry blur_geo(geo_absolute.x, geo_absolute.y, base.width, base.height);
      nux::ObjectPtr<nux::IOpenGLBaseTexture> blur_texture;

      if (BackgroundEffectHelper::blur_type != unity::BLUR_NONE && (bkg_box.x + bkg_box.width > 0))
      {
	blur_texture = bg_effect_helper_.GetBlurRegion(blur_geo);
      }
      else
      {
	blur_texture = bg_effect_helper_.GetRegion(blur_geo);
      }

      if (blur_texture.IsValid())
      {
	nux::TexCoordXForm texxform_blur_bg;
	texxform_blur_bg.flip_v_coord = true;
	texxform_blur_bg.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
	texxform_blur_bg.uoffset = ((float) base.x) / geo_absolute.width;
	texxform_blur_bg.voffset = ((float) base.y) / geo_absolute.height;

	GfxContext.PushClippingRectangle(bkg_box);

#ifndef NUX_OPENGLES_20
	if (GfxContext.UsingGLSLCodePath())
	  gPainter.PushDrawCompositionLayer(GfxContext, base,
					    blur_texture,
					    texxform_blur_bg,
					    nux::color::White,
					    options()->background_color, nux::LAYER_BLEND_MODE_OVERLAY,
					    true, ROP);
	else
	  gPainter.PushDrawTextureLayer(GfxContext, base,
					blur_texture,
					texxform_blur_bg,
					nux::color::White,
					true,
					ROP);
#else
	  gPainter.PushDrawCompositionLayer(GfxContext, base,
					    blur_texture,
					    texxform_blur_bg,
					    nux::color::White,
					    options()->background_color, nux::LAYER_BLEND_MODE_OVERLAY,
					    true, ROP);
#endif
	GfxContext.PopClippingRectangle();

	push_count++;
      }

      unsigned int alpha = 0, src = 0, dest = 0;
      GfxContext.GetRenderStates().GetBlend(alpha, src, dest);

      // apply the darkening
      GfxContext.GetRenderStates().SetBlend(true, GL_ZERO, GL_SRC_COLOR);
      gPainter.Paint2DQuadColor(GfxContext, bkg_box, nux::Color(0.9f, 0.9f, 0.9f, 1.0f));
      GfxContext.GetRenderStates().SetBlend (alpha, src, dest);

      // apply the bg colour
#ifndef NUX_OPENGLES_20
      if (GfxContext.UsingGLSLCodePath() == false)
	gPainter.Paint2DQuadColor(GfxContext, bkg_box, options()->background_color);
#endif

      // apply the shine
      GfxContext.GetRenderStates().SetBlend(true, GL_DST_COLOR, GL_ONE);
      nux::TexCoordXForm texxform;
      texxform.SetTexCoordType(nux::TexCoordXForm::OFFSET_COORD);
      texxform.SetWrap(nux::TEXWRAP_CLAMP, nux::TEXWRAP_CLAMP);
      texxform.uoffset = (1.0f / launcher_sheen_->GetWidth()); // TODO (gord) don't use absolute values here
      texxform.voffset = (1.0f / launcher_sheen_->GetHeight()) * panel::Style::Instance().panel_height;
      GfxContext.QRP_1Tex(base.x, base.y, base.width, base.height,
			  launcher_sheen_->GetDeviceTexture(),
			  texxform,
			  nux::color::White);

      //reset the blend state
      GfxContext.GetRenderStates().SetBlend (alpha, src, dest);
    }
    else
    {
      nux::Color color = options()->background_color;
      color.alpha = options()->background_alpha;
      gPainter.Paint2DQuadColor(GfxContext, bkg_box, color);
    }
  }

  GfxContext.GetRenderStates().SetPremultipliedBlend(nux::SRC_OVER);

  icon_renderer->PreprocessIcons(args, base);
  EventLogic();

  /* draw launcher */
  for (rev_it = args.rbegin(); rev_it != args.rend(); ++rev_it)
  {
    if ((*rev_it).stick_thingy)
      gPainter.Paint2DQuadColor(GfxContext,
                                nux::Geometry(bkg_box.x, (*rev_it).render_center.y - 3, bkg_box.width, 2),
                                nux::Color(0xAAAAAAAA));

    if ((*rev_it).skip)
      continue;

    icon_renderer->RenderIcon(GfxContext, *rev_it, bkg_box, base);
  }

  if (!IsOverlayOpen())
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
                                            8),
                              nux::Color(0x70000000),
                              nux::Color(0x00000000),
                              nux::Color(0x00000000),
                              nux::Color(0x70000000));
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

long Launcher::PostLayoutManagement(long LayoutResult)
{
  View::PostLayoutManagement(LayoutResult);

  SetMousePosition(0, 0);

  return nux::SIZE_EQUAL_HEIGHT | nux::SIZE_EQUAL_WIDTH;
}

void Launcher::OnDragWindowAnimCompleted()
{
  HideDragWindow();
  EnsureAnimation();
}

bool Launcher::StartIconDragTimeout(int x, int y)
{
  // if we are still waiting…
  if (GetActionState() == ACTION_NONE)
  {
    SetIconUnderMouse(AbstractLauncherIcon::Ptr());
    _initial_drag_animation = true;
    StartIconDragRequest(x, y);
  }

  return false;
}

void Launcher::StartIconDragRequest(int x, int y)
{
  nux::Geometry const& abs_geo = GetAbsoluteGeometry();
  AbstractLauncherIcon::Ptr const& drag_icon = MouseIconIntersection(abs_geo.width / 2.0f, y);

  // FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
  // on an internal Launcher property then
  if (drag_icon && _last_button_press == 1 && drag_icon->position() == AbstractLauncherIcon::Position::FLOATING)
  {
    auto const& icon_center = drag_icon->GetCenter(monitor);
    x += abs_geo.x;
    y += abs_geo.y;

    SetActionState(ACTION_DRAG_ICON);
    StartIconDrag(drag_icon);
    UpdateDragWindowPosition(icon_center.x, icon_center.y);

    if (_initial_drag_animation)
    {
      _drag_window->SetAnimationTarget(x, y);
      _drag_window->StartQuickAnimation();
    }

    EnsureAnimation();
  }
  else
  {
    _drag_icon = nullptr;
    HideDragWindow();
  }
}

void Launcher::StartIconDrag(AbstractLauncherIcon::Ptr const& icon)
{
  using namespace std::placeholders;

  if (!icon)
    return;

  _hide_machine.SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, true);
  _drag_icon = icon;
  _drag_icon_position = _model->IconIndex(icon);

  HideDragWindow();
  _offscreen_drag_texture = nux::GetGraphicsDisplay()->GetGpuDevice()->CreateSystemCapableDeviceTexture(GetWidth(), GetWidth(), 1, nux::BITFMT_R8G8B8A8);
  _drag_window = new LauncherDragWindow(_offscreen_drag_texture,
                                        std::bind(&Launcher::RenderIconToTexture, this,
                                                  _1,
                                                  _drag_icon, _offscreen_drag_texture));
  ShowDragWindow();

  ubus_.SendMessage(UBUS_LAUNCHER_ICON_START_DND);
}

void Launcher::EndIconDrag()
{
  if (_drag_window)
  {
    AbstractLauncherIcon::Ptr hovered_icon;

    if (!_drag_window->Cancelled())
      hovered_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

    if (hovered_icon && hovered_icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH)
    {
      hovered_icon->SetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE, true);

      remove_request.emit(_drag_icon);

      HideDragWindow();
      EnsureAnimation();
    }
    else
    {
      if (!_drag_window->Cancelled() && _model->IconIndex(_drag_icon) != _drag_icon_position)
      {
        _drag_icon->Stick(false);
        _model->Save();
      }

      if (_drag_window->on_anim_completed.connected())
        _drag_window->on_anim_completed.disconnect();
      _drag_window->on_anim_completed = _drag_window->anim_completed.connect(sigc::mem_fun(this, &Launcher::OnDragWindowAnimCompleted));

      auto const& icon_center = _drag_icon->GetCenter(monitor);
      _drag_window->SetAnimationTarget(icon_center.x, icon_center.y),
      _drag_window->StartQuickAnimation();
    }
  }

  if (MouseBeyondDragThreshold())
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_THRESHOLD], &_times[TIME_DRAG_THRESHOLD], ANIM_DURATION_SHORT);

  _hide_machine.SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, false);
  ubus_.SendMessage(UBUS_LAUNCHER_ICON_END_DND);
}

void Launcher::ShowDragWindow()
{
  if (!_drag_window || _drag_window->IsVisible())
    return;

  _drag_window->GrabKeyboard();
  _drag_window->ShowWindow(true);
  _drag_window->PushToFront();

  bool is_before;
  AbstractLauncherIcon::Ptr const& closer = _model->GetClosestIcon(_drag_icon, is_before);
  _drag_window->drag_cancel_request.connect([this, closer, is_before] {
    if (is_before)
      _model->ReorderAfter(_drag_icon, closer);
    else
      _model->ReorderBefore(_drag_icon, closer, true);

    ResetMouseDragState();
  });
}

void Launcher::HideDragWindow()
{
  nux::Geometry const& abs_geo = GetAbsoluteGeometry();
  nux::Point const& mouse = nux::GetWindowCompositor().GetMousePosition();

  if (abs_geo.IsInside(mouse))
    mouse_enter.emit(mouse.x - abs_geo.x, mouse.y - abs_geo.y, 0, 0);

  if (!_drag_window)
    return;

  _drag_window->UnGrabKeyboard();
  _drag_window->ShowWindow(false);
  _drag_window = nullptr;
}

void Launcher::UpdateDragWindowPosition(int x, int y)
{
  if (!_drag_window)
    return;

  auto const& icon_geo = _drag_window->GetGeometry();
  _drag_window->SetBaseXY(x - icon_geo.width / 2, y - icon_geo.height / 2);

  if (!_drag_icon)
    return;

  auto const& launcher_geo = GetGeometry();
  auto const& hovered_icon = MouseIconIntersection((launcher_geo.x + launcher_geo.width) / 2.0, y - GetAbsoluteY());
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);
  float progress = DragThresholdProgress(current);

  if (hovered_icon && _drag_icon != hovered_icon)
  {
    if (progress >= 1.0f)
    {
      _model->ReorderSmart(_drag_icon, hovered_icon, true);
    }
    else if (progress == 0.0f)
    {
      _model->ReorderBefore(_drag_icon, hovered_icon, false);
    }
  }
  else if (!hovered_icon && progress == 0.0f)
  {
    // If no icon is hovered, then we can add our icon to the bottom
    for (auto it = _model->main_rbegin(); it != _model->main_rend(); ++it)
    {
      auto const& icon = *it;

      if (!icon->IsVisible() || !icon->IsVisibleOnMonitor(monitor))
        continue;

      if (y >= icon->GetCenter(monitor).y)
      {
        _model->ReorderAfter(_drag_icon, icon);
        break;
      }
    }
  }
}

void Launcher::ResetMouseDragState()
{
  if (GetActionState() == ACTION_DRAG_ICON)
    EndIconDrag();

  if (GetActionState() == ACTION_DRAG_LAUNCHER)
    _hide_machine.SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, false);

  SetActionState(ACTION_NONE);
  _dnd_delta_x = 0;
  _dnd_delta_y = 0;
  _last_button_press = 0;
  EnsureAnimation();
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

  MouseUpLogic(x, y, button_flags, key_flags);
  ResetMouseDragState();
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

  SetIconUnderMouse(AbstractLauncherIcon::Ptr());

  if (GetActionState() == ACTION_NONE)
  {
#ifdef USE_X11
    if (nux::Abs(_dnd_delta_y) >= nux::Abs(_dnd_delta_x))
    {
      _launcher_drag_delta += _dnd_delta_y;
      SetActionState(ACTION_DRAG_LAUNCHER);
      _hide_machine.SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, true);
    }
    else
    {
      // We we can safely start the icon drag, from the original mouse-down position
      sources_.Remove(START_DRAGICON_DURATION);
      StartIconDragRequest(x - _dnd_delta_x, y - _dnd_delta_y);
    }
#endif
  }
  else if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    _launcher_drag_delta += dy;
    ubus_.SendMessage(UBUS_LAUNCHER_END_DND);
  }
  else if (GetActionState() == ACTION_DRAG_ICON)
  {
    nux::Geometry const& geo = GetAbsoluteGeometry();
    UpdateDragWindowPosition(geo.x + x, geo.y + y);
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
  SetStateMouseOverLauncher(false);
  EventLogic();
  EnsureAnimation();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);
  tooltip_manager_.MouseMoved(_icon_under_mouse);

  if (!_hidden)
    UpdateChangeInMousePosition(dx, dy);

  // Every time the mouse moves, we check if it is inside an icon...
  EventLogic();
}

void Launcher::RecvMouseWheel(int /*x*/, int /*y*/, int wheel_delta, unsigned long /*button_flags*/, unsigned long key_flags)
{
  if (!_hovered)
    return;

  bool alt_pressed = nux::GetKeyModifierState(key_flags, nux::NUX_STATE_ALT);
  if (alt_pressed)
  {
    ScrollLauncher(wheel_delta);
  }
  else if (_icon_under_mouse)
  {
    auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
    auto scroll_direction = (wheel_delta < 0) ? AbstractLauncherIcon::ScrollDirection::DOWN : AbstractLauncherIcon::ScrollDirection::UP;
    _icon_under_mouse->PerformScroll(scroll_direction, timestamp);
  }
}

void Launcher::ScrollLauncher(int wheel_delta)
{
  if (wheel_delta < 0)
    // scroll down
    _launcher_drag_delta -= 25;
  else
    // scroll up
    _launcher_drag_delta += 25;

  EnsureAnimation();
}

#ifdef USE_X11

ui::EdgeBarrierSubscriber::Result Launcher::HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event)
{
  if (_hide_machine.GetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE))
  {
    return ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE;
  }

  nux::Geometry const& abs_geo = GetAbsoluteGeometry();

  bool apply_to_reveal = false;
  if (event->x >= abs_geo.x && event->x <= abs_geo.x + abs_geo.width)
  {
    if (!_hidden)
      return ui::EdgeBarrierSubscriber::Result::ALREADY_HANDLED;

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
    int root_x_return, root_y_return, win_x_return, win_y_return;
    unsigned int mask_return;
    Window root_return, child_return;
    Display *dpy = nux::GetGraphicsDisplay()->GetX11Display();

    if (XQueryPointer (dpy, DefaultRootWindow(dpy), &root_return, &child_return, &root_x_return,
                       &root_y_return, &win_x_return, &win_y_return, &mask_return))
    {
      if (mask_return & (Button1Mask | Button3Mask))
        return ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE;
    }
  }

  if (!apply_to_reveal)
    return ui::EdgeBarrierSubscriber::Result::IGNORED;

  if (!owner->IsFirstEvent())
    _hide_machine.AddRevealPressure(event->velocity);

  return ui::EdgeBarrierSubscriber::Result::HANDLED;
}

#endif

bool Launcher::IsInKeyNavMode() const
{
  return _hide_machine.GetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE);
}

void Launcher::EnterKeyNavMode()
{
  _hide_machine.SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, true);
  _hover_machine.SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, true);
  SaturateIcons();
}

void Launcher::ExitKeyNavMode()
{
  _hide_machine.SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, false);
  _hover_machine.SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, false);
}

void Launcher::RecvQuicklistOpened(nux::ObjectPtr<QuicklistView> const& quicklist)
{
  UScreen* uscreen = UScreen::GetDefault();
  if (uscreen->GetMonitorGeometry(monitor).IsInside(nux::Point(quicklist->GetGeometry().x, quicklist->GetGeometry().y)))
  {
    _hide_machine.SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, true);
    _hover_machine.SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, true);
    EventLogic();
    EnsureAnimation();
  }
}

void Launcher::RecvQuicklistClosed(nux::ObjectPtr<QuicklistView> const& quicklist)
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

  _hide_machine.SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, false);
  _hover_machine.SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, false);

  EventLogic();
  EnsureAnimation();
}

void Launcher::EventLogic()
{
  if (GetActionState() == ACTION_DRAG_ICON ||
      GetActionState() == ACTION_DRAG_LAUNCHER)
    return;

  AbstractLauncherIcon::Ptr launcher_icon;

  if (!_hidden && !IsInKeyNavMode() && _hovered)
  {
    launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);
  }

  SetIconUnderMouse(launcher_icon);
}

void Launcher::MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon::Ptr const& launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  if (launcher_icon)
  {
    _icon_mouse_down = launcher_icon;
    // if MouseUp after the time ended -> it's an icon drag, otherwise, it's starting an app
    auto cb_func = sigc::bind(sigc::mem_fun(this, &Launcher::StartIconDragTimeout), x, y);
    sources_.AddTimeout(START_DRAGICON_DURATION, cb_func, START_DRAGICON_TIMEOUT);

    launcher_icon->mouse_down.emit(nux::GetEventButton(button_flags), monitor, key_flags);
    tooltip_manager_.IconClicked();
  }
}

void Launcher::MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon::Ptr const& launcher_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  sources_.Remove(START_DRAGICON_TIMEOUT);

  if (_icon_mouse_down && (_icon_mouse_down == launcher_icon))
  {
    _icon_mouse_down->mouse_up.emit(nux::GetEventButton(button_flags), monitor, key_flags);

    if (GetActionState() == ACTION_NONE)
    {
      _icon_mouse_down->mouse_click.emit(nux::GetEventButton(button_flags), monitor, key_flags);
    }
  }

  if (launcher_icon && (_icon_mouse_down != launcher_icon))
  {
    launcher_icon->mouse_up.emit(nux::GetEventButton(button_flags), monitor, key_flags);
  }

  if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    TimeUtil::SetTimeStruct(&_times[TIME_DRAG_END]);
  }

  _icon_mouse_down = nullptr;
}

AbstractLauncherIcon::Ptr Launcher::MouseIconIntersection(int x, int y) const
{
  LauncherModel::iterator it;
  // We are looking for the icon at screen coordinates x, y;
  nux::Point2 mouse_position(x, y);
  int inside = 0;

  for (it = _model->begin(); it != _model->end(); ++it)
  {
    if (!(*it)->IsVisible() || !(*it)->IsVisibleOnMonitor(monitor))
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

  return AbstractLauncherIcon::Ptr();
}

void Launcher::RenderIconToTexture(nux::GraphicsEngine& GfxContext, AbstractLauncherIcon::Ptr const& icon, nux::ObjectPtr<nux::IOpenGLBaseTexture> texture)
{
  RenderArg arg;
  struct timespec current;
  clock_gettime(CLOCK_MONOTONIC, &current);

  SetupRenderArg(icon, current, arg);
  arg.render_center = nux::Point3(roundf(texture->GetWidth() / 2.0f), roundf(texture->GetHeight() / 2.0f), 0.0f);
  arg.logical_center = arg.render_center;
  arg.rotation.x = 0.0f;
  arg.running_arrow = false;
  arg.active_arrow = false;
  arg.skip = false;
  arg.window_indicators = 0;
  arg.alpha = 1.0f;

  std::list<RenderArg> drag_args;
  drag_args.push_front(arg);

  graphics::PushOffscreenRenderTarget(texture);

  unsigned int alpha = 0, src = 0, dest = 0;
  GfxContext.GetRenderStates().GetBlend(alpha, src, dest);
  GfxContext.GetRenderStates().SetBlend(false);

  GfxContext.QRP_Color(0,
                      0,
                      texture->GetWidth(),
                      texture->GetHeight(),
                      nux::Color(0.0f, 0.0f, 0.0f, 0.0f));

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  nux::Geometry geo(0, 0, texture->GetWidth(), texture->GetWidth());

  icon_renderer->PreprocessIcons(drag_args, geo);
  icon_renderer->RenderIcon(GfxContext, arg, geo, geo);
  unity::graphics::PopOffscreenRenderTarget();
}

#ifdef NUX_GESTURES_SUPPORT
nux::GestureDeliveryRequest Launcher::GestureEvent(const nux::GestureEvent &event)
{
  switch(event.type)
  {
    case nux::EVENT_GESTURE_BEGIN:
      OnDragStart(event);
      break;
    case nux::EVENT_GESTURE_UPDATE:
      OnDragUpdate(event);
      break;
    default: // EVENT_GESTURE_END
      OnDragFinish(event);
      break;
  }

  return nux::GestureDeliveryRequest::NONE;
}
#endif

bool Launcher::DndIsSpecialRequest(std::string const& uri) const
{
  return (boost::algorithm::ends_with(uri, ".desktop") || uri.find("device://") == 0);
}

void Launcher::ProcessDndEnter()
{
#ifdef USE_X11
  SetStateMouseOverLauncher(true);

  _dnd_data.Reset();
  _drag_action = nux::DNDACTION_NONE;
  _steal_drag = false;
  _data_checked = false;
  _drag_edge_touching = false;
  _dnd_hovered_icon = nullptr;
#endif
}

void Launcher::DndReset()
{
#ifdef USE_X11
  _dnd_data.Reset();

  bool is_overlay_open = IsOverlayOpen();

  for (auto it : *_model)
  {
    auto icon_type = it->GetIconType();

    if (icon_type == AbstractLauncherIcon::IconType::HOME ||
        icon_type == AbstractLauncherIcon::IconType::HUD)
    {
      it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false);
    }
    else
    {
      it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, is_overlay_open && !_hovered);
    }

    it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, false);
  }

  DndHoveredIconReset();
#endif
}

void Launcher::DndHoveredIconReset()
{
#ifdef USE_X11
  _drag_edge_touching = false;
  SetActionState(ACTION_NONE);

  if (_steal_drag && _dnd_hovered_icon)
  {
    _dnd_hovered_icon->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
    _dnd_hovered_icon->remove.emit(_dnd_hovered_icon);
  }

  if (!_steal_drag && _dnd_hovered_icon)
    _dnd_hovered_icon->SendDndLeave();

  _steal_drag = false;
  _dnd_hovered_icon = nullptr;
#endif
}

void Launcher::ProcessDndLeave()
{
#ifdef USE_X11
  SetStateMouseOverLauncher(false);

  DndHoveredIconReset();
#endif
}

void Launcher::ProcessDndMove(int x, int y, std::list<char*> mimes)
{
#ifdef USE_X11
  if (!_data_checked)
  {
    const std::string uri_list = "text/uri-list";
    _data_checked = true;
    _dnd_data.Reset();
    auto& display = nux::GetWindowThread()->GetGraphicsDisplay();

    // get the data
    for (auto const& mime : mimes)
    {
      if (mime != uri_list)
        continue;

      _dnd_data.Fill(display.GetDndData(const_cast<char*>(uri_list.c_str())));
      break;
    }

    // see if the launcher wants this one
    auto const& uris = _dnd_data.Uris();
    if (std::find_if(uris.begin(), uris.end(), [this] (std::string const& uri)
                     {return DndIsSpecialRequest(uri);}) != uris.end())
    {
      _steal_drag = true;
    }

    // only set hover once we know our first x/y
    SetActionState(ACTION_DRAG_EXTERNAL);
    SetStateMouseOverLauncher(true);

    if (!_steal_drag && !_dnd_data.Uris().empty())
    {
      for (auto const& it : *_model)
      {
        if (it->ShouldHighlightOnDrag(_dnd_data))
          it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false);
        else
          it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true);
      }
    }
  }

  SetMousePosition(x - _parent->GetGeometry().x, y - _parent->GetGeometry().y);

  if (monitor() == 0 && !IsOverlayOpen() && _mouse_position.x == 0 && _mouse_position.y <= (_parent->GetGeometry().height - _icon_size - 2 * _space_between_icons) && !_drag_edge_touching)
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
  AbstractLauncherIcon::Ptr const& hovered_icon = MouseIconIntersection(_mouse_position.x, _mouse_position.y);

  bool hovered_icon_is_appropriate = false;
  if (hovered_icon)
  {
    if (hovered_icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH)
      _steal_drag = false;

    if (hovered_icon->position() == AbstractLauncherIcon::Position::FLOATING)
      hovered_icon_is_appropriate = true;
  }

  if (_steal_drag)
  {
    _drag_action = nux::DNDACTION_COPY;
    if (!_dnd_hovered_icon && hovered_icon_is_appropriate)
    {
      _dnd_hovered_icon = new SpacerLauncherIcon(monitor());
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
          _dnd_hovered_icon->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false);
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
#endif
}

void Launcher::ProcessDndDrop(int x, int y)
{
#ifdef USE_X11
  if (_steal_drag)
  {
    for (auto const& uri : _dnd_data.Uris())
    {
      if (DndIsSpecialRequest(uri))
        add_request.emit(uri, _dnd_hovered_icon);
    }
  }
  else if (_dnd_hovered_icon && _drag_action != nux::DNDACTION_NONE)
  {
     if (IsOverlayOpen())
       ubus_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);

    _dnd_hovered_icon->AcceptDrop(_dnd_data);
  }

  if (_drag_action != nux::DNDACTION_NONE)
    SendDndFinished(true, _drag_action);
  else
    SendDndFinished(false, _drag_action);

  // reset our shiz
  DndReset();
#endif
}

/*
 * Returns the current selected icon if it is in keynavmode
 * It will return NULL if it is not on keynavmode
 */
AbstractLauncherIcon::Ptr Launcher::GetSelectedMenuIcon() const
{
  if (!IsInKeyNavMode())
    return AbstractLauncherIcon::Ptr();

  return _model->Selection();
}

//
// Key navigation
//
bool Launcher::InspectKeyEvent(unsigned int eventType,
                          unsigned int keysym,
                          const char* character)
{
  // The Launcher accepts all key inputs.
  return true;
}

int Launcher::GetDragDelta() const
{
  return _launcher_drag_delta;
}

void Launcher::DndStarted(std::string const& data)
{
#ifdef USE_X11
  SetDndQuirk();

  _dnd_data.Fill(data.c_str());

  auto const& uris = _dnd_data.Uris();
  if (std::find_if(uris.begin(), uris.end(), [this] (std::string const& uri)
                   {return DndIsSpecialRequest(uri);}) != uris.end())
  {
    _steal_drag = true;

    if (IsOverlayOpen())
      SaturateIcons();
  }
  else
  {
    for (auto const& it : *_model)
    {
      if (it->ShouldHighlightOnDrag(_dnd_data))
      {
        it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false);
        it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, true);
      }
      else
      {
        it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true);
        it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, false);
      }
    }
  }
#endif
}

void Launcher::DndFinished()
{
#ifdef USE_X11
  UnsetDndQuirk();

  _data_checked = false;

  if (IsOverlayOpen() && !_hovered)
    DesaturateIcons();

  DndReset();
#endif
}

void Launcher::SetDndQuirk()
{
#ifdef USE_X11
  _hide_machine.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, true);
#endif
}

void Launcher::UnsetDndQuirk()
{
#ifdef USE_X11
  _hide_machine.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
  _hide_machine.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
#endif
}

} // namespace launcher
} // namespace unity
