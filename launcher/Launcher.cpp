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

#include <Nux/Nux.h>
#include <NuxCore/Logger.h>

#include "Launcher.h"
#include "AbstractLauncherIcon.h"
#include "SpacerLauncherIcon.h"
#include "LauncherModel.h"
#include "QuicklistManager.h"
#include "QuicklistView.h"
#include "unity-shared/AnimationUtils.h"
#include "unity-shared/IconRenderer.h"
#include "unity-shared/GraphicsUtils.h"
#include "unity-shared/IconLoader.h"
#include "unity-shared/PanelStyle.h"
#include "unity-shared/TextureCache.h"
#include "unity-shared/TimeUtil.h"
#include "unity-shared/UScreen.h"
#include "unity-shared/UBusMessages.h"
#include "unity-shared/UnitySettings.h"

#include <UnityCore/GLibWrapper.h>
#include <UnityCore/Variant.h>

#include <boost/algorithm/string.hpp>

namespace unity
{
using ui::RenderArg;

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

const int ANIM_DURATION_SHORT = 125;
const int ANIM_DURATION_SHORT_SHORT = 100;
const int ANIM_DURATION = 200;
const int ANIM_DURATION_LONG = 350;
const int START_DRAGICON_DURATION = 250;

const int DEFAULT_ICON_SIZE = 48;
const int DEFAULT_ICON_SIZE_DELTA = 6;
const int SPACE_BETWEEN_ICONS = 5;

const int MOUSE_DEADZONE = 15;

const float DRAG_OUT_PIXELS = 300.0f;
const float FOLDED_Z_DISTANCE = 10.f;
const float NEG_FOLDED_ANGLE = -1.0f;

const int SCROLL_AREA_HEIGHT = 24;
const int SCROLL_FPS = 30;

const int BASE_URGENT_ANIMATION_PERIOD = 6; // In seconds
const int MAX_URGENT_ANIMATION_DELTA = 960;  // In seconds
const int DOUBLE_TIME = 2;

const std::string START_DRAGICON_TIMEOUT = "start-dragicon-timeout";
const std::string SCROLL_TIMEOUT = "scroll-timeout";
const std::string SCALE_DESATURATE_IDLE = "scale-desaturate-idle";
const std::string URGENT_TIMEOUT = "urgent-timeout";
const std::string LAST_ANIMATION_URGENT_IDLE = "last-animation-urgent-idle";
}

NUX_IMPLEMENT_OBJECT_TYPE(Launcher);

Launcher::Launcher(MockableBaseWindow* parent,
                   NUX_FILE_LINE_DECL)
  : View(NUX_FILE_LINE_PARAM)
  , monitor(0)
  , parent_(parent)
  , icon_renderer_(std::make_shared<ui::IconRenderer>())
  , hovered_(false)
  , hidden_(false)
  , folded_(true)
  , shortcuts_shown_(false)
  , data_checked_(false)
  , steal_drag_(false)
  , drag_edge_touching_(false)
  , initial_drag_animation_(false)
  , dash_is_open_(false)
  , hud_is_open_(false)
  , launcher_action_state_(ACTION_NONE)
  , icon_size_(DEFAULT_ICON_SIZE + DEFAULT_ICON_SIZE_DELTA)
  , dnd_delta_y_(0)
  , dnd_delta_x_(0)
  , postreveal_mousemove_delta_x_(0)
  , postreveal_mousemove_delta_y_(0)
  , launcher_drag_delta_(0)
  , launcher_drag_delta_max_(0)
  , launcher_drag_delta_min_(0)
  , enter_y_(0)
  , last_button_press_(0)
  , urgent_animation_period_(0)
  , urgent_ack_needed_(false)
  , drag_out_delta_x_(0.0f)
  , drag_gesture_ongoing_(false)
  , last_reveal_progress_(0.0f)
  , drag_action_(nux::DNDACTION_NONE)
  , auto_hide_animation_(ANIM_DURATION_SHORT)
  , hover_animation_(ANIM_DURATION)
  , drag_over_animation_(ANIM_DURATION_LONG)
  , drag_out_animation_(ANIM_DURATION_SHORT)
  , drag_icon_animation_(ANIM_DURATION_SHORT)
  , dnd_hide_animation_(ANIM_DURATION * 3)
  , dash_showing_animation_(90)
{
  icon_renderer_->monitor = monitor();
  icon_renderer_->SetTargetSize(icon_size_, DEFAULT_ICON_SIZE, SPACE_BETWEEN_ICONS);

  bg_effect_helper_.owner = this;
  bg_effect_helper_.enabled = false;

  CaptureMouseDownAnyWhereElse(true);
  SetAcceptKeyNavFocusOnMouseDown(false);
  SetAcceptMouseWheelEvent(true);
  SetDndEnabled(false, true);

  auto const& redraw_cb = sigc::hide(sigc::mem_fun(this, &Launcher::QueueDraw));
  hide_machine_.should_hide_changed.connect(sigc::mem_fun(this, &Launcher::SetHidden));
  hide_machine_.reveal_progress.changed.connect(redraw_cb);
  hover_machine_.should_hover_changed.connect(sigc::mem_fun(this, &Launcher::SetHover));

  mouse_down.connect(sigc::mem_fun(this, &Launcher::RecvMouseDown));
  mouse_up.connect(sigc::mem_fun(this, &Launcher::RecvMouseUp));
  mouse_drag.connect(sigc::mem_fun(this, &Launcher::RecvMouseDrag));
  mouse_enter.connect(sigc::mem_fun(this, &Launcher::RecvMouseEnter));
  mouse_leave.connect(sigc::mem_fun(this, &Launcher::RecvMouseLeave));
  mouse_move.connect(sigc::mem_fun(this, &Launcher::RecvMouseMove));
  mouse_wheel.connect(sigc::mem_fun(this, &Launcher::RecvMouseWheel));

  QuicklistManager& ql_manager = *(QuicklistManager::Default());
  ql_manager.quicklist_opened.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistOpened));
  ql_manager.quicklist_closed.connect(sigc::mem_fun(this, &Launcher::RecvQuicklistClosed));

  WindowManager& wm = WindowManager::Default();
  wm.initiate_spread.connect(sigc::mem_fun(this, &Launcher::OnSpreadChanged));
  wm.terminate_spread.connect(sigc::mem_fun(this, &Launcher::OnSpreadChanged));
  wm.initiate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.terminate_expo.connect(sigc::mem_fun(this, &Launcher::OnPluginStateChanged));
  wm.screen_viewport_switch_ended.connect(sigc::mem_fun(this, &Launcher::QueueDraw));

  ubus_.RegisterInterest(UBUS_OVERLAY_SHOWN, sigc::mem_fun(this, &Launcher::OnOverlayShown));
  ubus_.RegisterInterest(UBUS_OVERLAY_HIDDEN, sigc::mem_fun(this, &Launcher::OnOverlayHidden));
  ubus_.RegisterInterest(UBUS_LAUNCHER_LOCK_HIDE, sigc::mem_fun(this, &Launcher::OnLockHideChanged));

  TextureCache& cache = TextureCache::GetDefault();
  launcher_sheen_ = cache.FindTexture("dash_sheen.png");
  launcher_pressure_effect_ = cache.FindTexture("launcher_pressure_effect.png");

  options.changed.connect(sigc::mem_fun(this, &Launcher::OnOptionsChanged));
  monitor.changed.connect(sigc::mem_fun(this, &Launcher::OnMonitorChanged));

  auto_hide_animation_.updated.connect(redraw_cb);
  hover_animation_.updated.connect(redraw_cb);
  drag_over_animation_.updated.connect(redraw_cb);
  drag_out_animation_.updated.connect(redraw_cb);
  drag_icon_animation_.updated.connect(redraw_cb);
  dnd_hide_animation_.updated.connect(redraw_cb);
  dash_showing_animation_.updated.connect(redraw_cb);
}

/* Introspection */
std::string Launcher::GetName() const
{
  return "Launcher";
}

#ifdef NUX_GESTURES_SUPPORT
void Launcher::OnDragStart(const nux::GestureEvent &event)
{
  drag_gesture_ongoing_ = true;
  if (hidden_)
  {
    drag_out_delta_x_ = 0.0f;
  }
  else
  {
    drag_out_delta_x_ = DRAG_OUT_PIXELS;
    hide_machine_.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
  }
}

void Launcher::OnDragUpdate(const nux::GestureEvent &event)
{
  drag_out_delta_x_ =
    CLAMP(drag_out_delta_x_ + event.GetDelta().x, 0.0f, DRAG_OUT_PIXELS);
  QueueDraw();
}

void Launcher::OnDragFinish(const nux::GestureEvent &event)
{
  if (drag_out_delta_x_ >= DRAG_OUT_PIXELS - 90.0f)
    hide_machine_.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, true);

  animation::StartOrReverse(drag_out_animation_, animation::Direction::BACKWARD);
  drag_gesture_ongoing_ = false;
}
#endif

void Launcher::AddProperties(GVariantBuilder* builder)
{
  unity::variant::BuilderWrapper(builder)
  .add(GetAbsoluteGeometry())
  .add("hover-progress", hover_animation_.GetCurrentValue())
  .add("dnd-exit-progress", drag_over_animation_.GetCurrentValue())
  .add("autohide-progress", auto_hide_animation_.GetCurrentValue())
  .add("dnd-delta", dnd_delta_y_)
  .add("hovered", hovered_)
  .add("hidemode", options()->hide_mode)
  .add("hidden", hidden_)
  .add("is_showing", ! hidden_)
  .add("monitor", monitor())
  .add("quicklist-open", hide_machine_.GetQuirk(LauncherHideMachine::QUICKLIST_OPEN))
  .add("hide-quirks", hide_machine_.DebugHideQuirks())
  .add("hover-quirks", hover_machine_.DebugHoverQuirks())
  .add("icon-size", icon_size_)
  .add("shortcuts_shown", shortcuts_shown_)
  .add("tooltip-shown", active_tooltip_ != nullptr);
}

void Launcher::SetMousePosition(int x, int y)
{
  bool was_beyond_drag_threshold = MouseBeyondDragThreshold();
  mouse_position_ = nux::Point(x, y);
  bool is_beyond_drag_threshold = MouseBeyondDragThreshold();

  if (was_beyond_drag_threshold != is_beyond_drag_threshold)
    animation::StartOrReverseIf(drag_icon_animation_, !is_beyond_drag_threshold);

  EnsureScrollTimer();
}

void Launcher::SetStateMouseOverLauncher(bool over_launcher)
{
  hide_machine_.SetQuirk(LauncherHideMachine::MOUSE_OVER_LAUNCHER, over_launcher);
  hide_machine_.SetQuirk(LauncherHideMachine::REVEAL_PRESSURE_PASS, false);
  hover_machine_.SetQuirk(LauncherHoverMachine::MOUSE_OVER_LAUNCHER, over_launcher);
  tooltip_manager_.SetHover(over_launcher);
}

void Launcher::SetIconUnderMouse(AbstractLauncherIcon::Ptr const& icon)
{
  if (icon_under_mouse_ == icon)
    return;

  if (icon_under_mouse_)
    icon_under_mouse_->mouse_leave.emit(monitor);
  if (icon)
    icon->mouse_enter.emit(monitor);

  icon_under_mouse_ = icon;
}

bool Launcher::MouseBeyondDragThreshold() const
{
  if (GetActionState() == ACTION_DRAG_ICON)
    return mouse_position_.x > GetGeometry().width + icon_size_ / 2;
  return false;
}

/* Render Layout Logic */
float Launcher::DragOutProgress() const
{
  float progress = drag_out_delta_x_ / DRAG_OUT_PIXELS;

  if (drag_gesture_ongoing_ || hide_machine_.GetQuirk(LauncherHideMachine::MT_DRAG_OUT))
    return progress;
  else
    return progress * drag_out_animation_.GetCurrentValue();
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

void Launcher::SetDndDelta(float x, float y, nux::Geometry const& geo)
{
  auto const& anchor = MouseIconIntersection(x, enter_y_);

  if (anchor)
  {
    float position = y;
    for (AbstractLauncherIcon::Ptr const& model_icon : *model_)
    {
      if (model_icon == anchor)
      {
        position += icon_size_ / 2;
        launcher_drag_delta_ = enter_y_ - position;

        if (position + icon_size_ / 2 + launcher_drag_delta_ > geo.height)
          launcher_drag_delta_ -= (position + icon_size_ / 2 + launcher_drag_delta_) - geo.height;

        break;
      }
      float visibility = model_icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::VISIBLE, monitor());
      position += (icon_size_ + SPACE_BETWEEN_ICONS) * visibility;
    }
  }
}

float Launcher::IconUrgentPulseValue(AbstractLauncherIcon::Ptr const& icon) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()))
    return 1.0f; // we are full on in a normal condition

  float urgent_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::URGENT, monitor());
  return 0.5f + (float)(std::cos(M_PI * (float)(URGENT_BLINKS * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconPulseOnceValue(AbstractLauncherIcon::Ptr const& icon) const
{
  float pulse_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::PULSE_ONCE, monitor());

  if (pulse_progress == 1.0f)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE, false, monitor());
    icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::PULSE_ONCE, monitor());
  }

  return 0.5f + (float) (std::cos(M_PI * 2.0 * pulse_progress)) * 0.5f;
}

float Launcher::IconUrgentWiggleValue(AbstractLauncherIcon::Ptr const& icon) const
{
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()))
    return 0.0f; // we are full on in a normal condition

  float urgent_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::URGENT, monitor());
  return 0.3f * (float)(std::sin(M_PI * (float)(WIGGLE_CYCLES * 2) * urgent_progress)) * 0.5f;
}

float Launcher::IconStartingBlinkValue(AbstractLauncherIcon::Ptr const& icon) const
{
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING, monitor()))
    return 1.0f;

  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::STARTING, monitor()))
    return 1.0f;

  float starting_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::STARTING, monitor());

  double val = IsBackLightModeToggles() ? 3.0f : 4.0f;
  return 1.0f-(0.5f + (float)(std::cos(M_PI * val * starting_progress)) * 0.5f);
}

float Launcher::IconStartingPulseValue(AbstractLauncherIcon::Ptr const& icon) const
{
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING, monitor()))
    return 1.0f;

  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::STARTING, monitor()))
    return 1.0f;

  float starting_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::STARTING, monitor());

  if (starting_progress == 1.0f)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, false, monitor());
    icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::STARTING, monitor());
    return starting_progress;
  }

  return 1.0f-(0.5f + (float)(std::cos(M_PI * (float)(MAX_STARTING_BLINKS * 2) * starting_progress)) * 0.5f);
}

float Launcher::IconBackgroundIntensity(AbstractLauncherIcon::Ptr const& icon) const
{
  float result = 0.0f;
  float running_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::RUNNING, monitor());

  // After we finish the running animation running, we can restore the starting quirk.
  if (running_progress == 1.0f && icon->GetQuirk(AbstractLauncherIcon::Quirk::STARTING, monitor()))
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::STARTING, false, monitor());
    icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::STARTING, monitor());
  }

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
        result = IconStartingBlinkValue(icon);
      else if (options()->backlight_mode() == BACKLIGHT_ALWAYS_OFF)
        result = 1.0f - IconStartingBlinkValue(icon);
      else
        result = backlight_strength; // The blink concept is a failure in this case (it just doesn't work right)
      break;
    case LAUNCH_ANIMATION_PULSE:
      result = backlight_strength;
      if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
        result *= CLAMP(running_progress + IconStartingPulseValue(icon), 0.0f, 1.0f);
      else if (IsBackLightModeToggles())
        result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconStartingPulseValue(icon));
      else
        result = 1.0f - CLAMP(running_progress + IconStartingPulseValue(icon), 0.0f, 1.0f);
      break;
  }

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE, monitor()))
  {
    if (options()->backlight_mode() == BACKLIGHT_ALWAYS_ON)
      result *= CLAMP(running_progress + IconPulseOnceValue(icon), 0.0f, 1.0f);
    else if (options()->backlight_mode() == BACKLIGHT_NORMAL)
      result += (BACKLIGHT_STRENGTH - result) * (1.0f - IconPulseOnceValue(icon));
    else
      result = 1.0f - CLAMP(running_progress + IconPulseOnceValue(icon), 0.0f, 1.0f);
  }

  // urgent serves to bring the total down only
  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()) && options()->urgent_animation() == URGENT_ANIMATION_PULSE)
    result *= 0.2f + 0.8f * IconUrgentPulseValue(icon);

  return result;
}

float Launcher::IconProgressBias(AbstractLauncherIcon::Ptr const& icon) const
{
  float result = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::PROGRESS, monitor());

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::PROGRESS, monitor()))
    return -1.0f + result;
  else
    return 1.0f - result;
}

bool Launcher::IconDrawEdgeOnly(AbstractLauncherIcon::Ptr const& icon) const
{
  if (options()->backlight_mode() == BACKLIGHT_EDGE_TOGGLE)
    return true;

  if (options()->backlight_mode() == BACKLIGHT_NORMAL_EDGE_TOGGLE && !icon->WindowVisibleOnMonitor(monitor))
    return true;

  return false;
}

void Launcher::SetupRenderArg(AbstractLauncherIcon::Ptr const& icon, RenderArg& arg)
{
  float stauration        = 1.0f - icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::DESAT, monitor());
  arg.icon                = icon.GetPointer();
  arg.alpha               = 0.2f + 0.8f * stauration;
  arg.saturation          = stauration;
  arg.colorify            = nux::color::White;
  arg.running_arrow       = icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING, monitor());
  arg.running_colored     = icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor());
  arg.draw_edge_only      = IconDrawEdgeOnly(icon);
  arg.active_colored      = false;
  arg.skip                = false;
  arg.stick_thingy        = false;
  arg.keyboard_nav_hl     = false;
  arg.progress_bias       = IconProgressBias(icon);
  arg.progress            = CLAMP(icon->GetProgress(), 0.0f, 1.0f);
  arg.draw_shortcut       = shortcuts_shown_ && !hide_machine_.GetQuirk(LauncherHideMachine::PLACES_VISIBLE);
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

  arg.active_arrow = icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, monitor());

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
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::RUNNING, monitor()))
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

  arg.backlight_intensity = IconBackgroundIntensity(icon);
  arg.shimmer_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::SHIMMER, monitor());

  float urgent_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::URGENT, monitor());

  if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()))
    urgent_progress = CLAMP(urgent_progress * 3.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle
  else
    urgent_progress = CLAMP(urgent_progress * 3.0f - 2.0f, 0.0f, 1.0f);  // we want to go 3x faster than the urgent normal cycle

  arg.glow_intensity = urgent_progress;

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
  {
    arg.rotation.z = IconUrgentWiggleValue(icon);
  }

  if (IsInKeyNavMode())
  {
    if (icon == model_->Selection())
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
                             float animation_neg_rads)
{
  SetupRenderArg(icon, arg);

  // reset z
  center.z = 0;

  float size_modifier = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::VISIBLE, monitor());
  if (size_modifier < 1.0f)
  {
    arg.alpha *= size_modifier;
    center.z = 300.0f * (1.0f - size_modifier);
  }

  float icon_dim = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::DROP_DIM, monitor());
  float drop_dim_value = 0.2f + 0.8f * (1.0f - icon_dim);

  if (drop_dim_value < 1.0f)
    arg.alpha *= drop_dim_value;

  // trying to protect against flickering when icon is dragged from dash LP: #863230
  if (arg.alpha < 0.2)
  {
    arg.alpha = 0.2;
    arg.saturation = 0.0;
  }

  if (icon == drag_icon_)
  {
    bool mouse_beyond_drag_threshold = MouseBeyondDragThreshold();

    if (mouse_beyond_drag_threshold)
      arg.stick_thingy = true;

    if (GetActionState() == ACTION_DRAG_ICON ||
        (drag_window_ && drag_window_->Animating()) ||
        icon->GetIconType() == AbstractLauncherIcon::IconType::SPACER)
    {
      arg.skip = true;
    }

    if (drag_icon_animation_.CurrentState() == na::Animation::State::Running)
      size_modifier *= drag_icon_animation_.GetCurrentValue();
    else if (mouse_beyond_drag_threshold)
      size_modifier = 0.0f;
  }

  if (size_modifier <= 0.0f)
    arg.skip = true;

  // goes for 0.0f when fully unfolded, to 1.0f folded
  float folding_progress = CLAMP((center.y + icon_size_ - folding_threshold) / (float) icon_size_, 0.0f, 1.0f);
  float unfold_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::UNFOLDED, monitor());

  folding_progress *= 1.0f - unfold_progress;

  float half_size = (folded_size / 2.0f) + (icon_size_ / 2.0f - folded_size / 2.0f) * (1.0f - folding_progress);
  float icon_hide_offset = autohide_offset;

  float present_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::PRESENTED, monitor());
  icon_hide_offset *= 1.0f - (present_progress * icon->PresentUrgency());

  // icon is crossing threshold, start folding
  center.z += folded_z_distance * folding_progress;
  arg.rotation.x = animation_neg_rads * folding_progress;

  float spacing_overlap = CLAMP((float)(center.y + (2.0f * half_size * size_modifier) + (SPACE_BETWEEN_ICONS * size_modifier) - folding_threshold) / (float) icon_size_, 0.0f, 1.0f);
  float spacing = (SPACE_BETWEEN_ICONS * (1.0f - spacing_overlap) + folded_spacing * spacing_overlap) * size_modifier;

  nux::Point3 centerOffset;
  float center_transit_progress = icon->GetQuirkProgress(AbstractLauncherIcon::Quirk::CENTER_SAVED, monitor());
  if (center_transit_progress <= 1.0f)
  {
    int saved_center = icon->GetSavedCenter(monitor).y - parent_abs_geo.y;
    centerOffset.y = (saved_center - (center.y + (half_size * size_modifier))) * (1.0f - center_transit_progress);
  }

  center.y += half_size * size_modifier;   // move to center

  arg.render_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y + centerOffset.y), roundf(center.z));
  arg.logical_center = nux::Point3(roundf(center.x + icon_hide_offset), roundf(center.y), roundf(center.z));

  nux::Point3 icon_center(parent_abs_geo.x + roundf(center.x), parent_abs_geo.y + roundf(center.y), roundf(center.z));
  icon->SetCenter(icon_center, monitor);

  // FIXME: this is a hack, to avoid that we set the target to the end of the icon
  if (!initial_drag_animation_ && icon == drag_icon_ && drag_window_ && drag_window_->Animating())
  {
    drag_window_->SetAnimationTarget(icon_center.x, icon_center.y);
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

nux::Color FullySaturateColor(nux::Color color)
{
  float max = std::max<float>({color.red, color.green, color.blue});

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

  nux::Color const& colorify = FullySaturateColor(options()->background_color);

  float hover_progress = folded_ ? hover_animation_.GetCurrentValue() : 1.0f;
  float folded_z_distance = FOLDED_Z_DISTANCE * (1.0f - hover_progress);
  float animation_neg_rads = NEG_FOLDED_ANGLE * (1.0f - hover_progress);

  float folding_constant = 0.25f;
  float folding_not_constant = folding_constant + ((1.0f - folding_constant) * hover_progress);

  float folded_size = icon_size_ * folding_not_constant;
  float folded_spacing = SPACE_BETWEEN_ICONS * folding_not_constant;

  center.x = geo.width / 2;
  center.y = SPACE_BETWEEN_ICONS;
  center.z = 0;

  int launcher_height = geo.height;
  folded_ = true;

  // compute required height of launcher AND folding threshold
  float sum = 0.0f + center.y;
  float folding_threshold = launcher_height - icon_size_ / 2.5f;
  for (it = model_->begin(); it != model_->end(); ++it)
  {
    float visibility = (*it)->GetQuirkProgress(AbstractLauncherIcon::Quirk::VISIBLE, monitor());
    float height = (icon_size_ + SPACE_BETWEEN_ICONS) * visibility;
    sum += height;

    // magic constant must some day be explained, for now suffice to say this constant prevents the bottom from "marching";
    float magic_constant = 1.3f;

    float unfold_progress = (*it)->GetQuirkProgress(AbstractLauncherIcon::Quirk::UNFOLDED, monitor());
    folding_threshold -= CLAMP(sum - launcher_height, 0.0f, height * magic_constant) * (folding_constant + (1.0f - folding_constant) * unfold_progress);
  }

  if (sum - SPACE_BETWEEN_ICONS <= launcher_height)
  {
    folding_threshold = launcher_height;
    folded_ = false;
  }

  float autohide_offset = 0.0f;
  *launcher_alpha = 1.0f;
  if (options()->hide_mode != LAUNCHER_HIDE_NEVER || hide_machine_.GetQuirk(LauncherHideMachine::LOCK_HIDE))
  {
    float autohide_progress = auto_hide_animation_.GetCurrentValue() * (1.0f - DragOutProgress());
    if (dash_is_open_)
    {
      *launcher_alpha = dash_showing_animation_.GetCurrentValue();
    }
    else if (options()->auto_hide_animation() == FADE_ONLY)
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

  float drag_hide_progress = dnd_hide_animation_.GetCurrentValue();
  if (options()->hide_mode != LAUNCHER_HIDE_NEVER && drag_hide_progress > 0.0f)
  {
    autohide_offset -= geo.width * 0.25f * drag_hide_progress;

    if (drag_hide_progress >= 1.0f)
      hide_machine_.SetQuirk(LauncherHideMachine::DND_PUSHED_OFF, true);
  }

  // Inform the painter where to paint the box
  box_geo = geo;

  if (options()->hide_mode != LAUNCHER_HIDE_NEVER || hide_machine_.GetQuirk(LauncherHideMachine::LOCK_HIDE))
    box_geo.x += autohide_offset;

  /* Why we need last_geo? It stores the last box_geo (note: as it is a static variable,
   * it is initialized only first time). Infact we call SetDndDelta that calls MouseIconIntersection
   * that uses values (HitArea) that are computed in UpdateIconXForm.
   * The problem is that in DrawContent we calls first RenderArgs, then UpdateIconXForm. Just
   * use last_geo to hack this problem.
   */
  static nux::Geometry last_geo = box_geo;

  // this happens on hover, basically its a flag and a value in one, we translate this into a dnd offset
  if (enter_y_ != 0 && enter_y_ + icon_size_ / 2 > folding_threshold)
    SetDndDelta(last_geo.x + last_geo.width / 2, center.y, geo);

  // Update the last_geo value.
  last_geo = box_geo;
  enter_y_ = 0;

  // logically dnd exit only restores to the clamped ranges
  // hover_progress restores to 0
  launcher_drag_delta_max_ = 0.0f;
  launcher_drag_delta_min_ = MIN(0.0f, launcher_height - sum);

  if (hover_progress > 0.0f && launcher_drag_delta_ != 0)
  {
    float delta_y = launcher_drag_delta_;

    if (launcher_drag_delta_ > launcher_drag_delta_max_)
      delta_y = launcher_drag_delta_max_ + DragLimiter(delta_y - launcher_drag_delta_max_);
    else if (launcher_drag_delta_ < launcher_drag_delta_min_)
      delta_y = launcher_drag_delta_min_ + DragLimiter(delta_y - launcher_drag_delta_min_);

    if (GetActionState() != ACTION_DRAG_LAUNCHER)
    {
      // XXX: nux::Animation should allow to define new kinds of easing curves
      float dnd_progress = std::pow(drag_over_animation_.GetCurrentValue(), 2);

      if (launcher_drag_delta_ > launcher_drag_delta_max_)
        delta_y = launcher_drag_delta_max_ + (delta_y - launcher_drag_delta_max_) * dnd_progress;
      else if (launcher_drag_delta_ < launcher_drag_delta_min_)
        delta_y = launcher_drag_delta_min_ + (delta_y - launcher_drag_delta_min_) * dnd_progress;

      if (dnd_progress == 0.0f)
        launcher_drag_delta_ = (int) delta_y;
    }

    delta_y *= hover_progress;
    center.y += delta_y;
    folding_threshold += delta_y;
  }
  else
  {
    launcher_drag_delta_ = 0;
  }

  // The functional position we wish to represent for these icons is not smooth. Rather than introducing
  // special casing to represent this, we use MIN/MAX functions. This helps ensure that even though our
  // function is not smooth it is continuous, which is more important for our visual representation (icons
  // wont start jumping around).  As a general rule ANY if () statements that modify center.y should be seen
  // as bugs.
  for (it = model_->main_begin(); it != model_->main_end(); ++it)
  {
    RenderArg arg;
    AbstractLauncherIcon::Ptr const& icon = *it;

    if (options()->hide_mode == LAUNCHER_HIDE_AUTOHIDE)
      HandleUrgentIcon(icon);

    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads);
    arg.colorify = colorify;
    launcher_args.push_back(arg);
  }

  // compute maximum height of shelf
  float shelf_sum = 0.0f;
  for (it = model_->shelf_begin(); it != model_->shelf_end(); ++it)
  {
    float visibility = (*it)->GetQuirkProgress(AbstractLauncherIcon::Quirk::VISIBLE, monitor());
    float height = (icon_size_ + SPACE_BETWEEN_ICONS) * visibility;
    shelf_sum += height;
  }

  // add bottom padding
  if (shelf_sum > 0.0f)
    shelf_sum += SPACE_BETWEEN_ICONS;

  float shelf_delta = MAX(((launcher_height - shelf_sum) + SPACE_BETWEEN_ICONS) - center.y, 0.0f);
  folding_threshold += shelf_delta;
  center.y += shelf_delta;

  for (it = model_->shelf_begin(); it != model_->shelf_end(); ++it)
  {
    RenderArg arg;
    AbstractLauncherIcon::Ptr const& icon = *it;

    FillRenderArg(icon, arg, center, parent_abs_geo, folding_threshold, folded_size, folded_spacing,
                  autohide_offset, folded_z_distance, animation_neg_rads);
    arg.colorify = colorify;
    launcher_args.push_back(arg);
  }
}

/* End Render Layout Logic */

void Launcher::ForceReveal(bool force_reveal)
{
  hide_machine_.SetQuirk(LauncherHideMachine::TRIGGER_BUTTON_SHOW, force_reveal);
}

void Launcher::ShowShortcuts(bool show)
{
  shortcuts_shown_ = show;
  hide_machine_.SetQuirk(LauncherHideMachine::SHORTCUT_KEYS_VISIBLE, show);
  QueueDraw();
}

void Launcher::OnLockHideChanged(GVariant *data)
{
  gboolean enable_lock = FALSE;
  g_variant_get(data, "(b)", &enable_lock);

  if (enable_lock)
  {
    hide_machine_.SetQuirk(LauncherHideMachine::LOCK_HIDE, true);
  }
  else
  {
    hide_machine_.SetQuirk(LauncherHideMachine::LOCK_HIDE, false);
  }
}

void Launcher::DesaturateIcons()
{
  bool inactive_only = WindowManager::Default().IsScaleActiveForGroup();

  for (auto const& icon : *model_)
  {
    bool desaturate = false;

    if (icon->GetIconType () != AbstractLauncherIcon::IconType::HOME &&
        icon->GetIconType () != AbstractLauncherIcon::IconType::HUD &&
        (!inactive_only || !icon->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, monitor())))
    {
      desaturate = true;
    }

    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, desaturate, monitor());
  }
}

void Launcher::SaturateIcons()
{
  for (auto const& icon : *model_)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false, monitor());
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
      dash_is_open_ = true;
      hide_machine_.SetQuirk(LauncherHideMachine::PLACES_VISIBLE, true);
      hover_machine_.SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, true);

      if (options()->hide_mode != LAUNCHER_HIDE_NEVER)
        animation::StartOrReverse(dash_showing_animation_, animation::Direction::FORWARD);
    }
    if (identity == "hud")
    {
      hud_is_open_ = true;
    }

    bg_effect_helper_.enabled = true;

    // Don't desaturate icons if the mouse is over the launcher:
    if (!hovered_)
    {
      LOG_DEBUG(logger) << "Desaturate on monitor " << monitor();
      DesaturateIcons();
    }

    if (icon_under_mouse_)
      icon_under_mouse_->HideTooltip();
  }
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
      dash_is_open_ = false;
      hide_machine_.SetQuirk(LauncherHideMachine::PLACES_VISIBLE, false);
      hover_machine_.SetQuirk(LauncherHoverMachine::PLACES_VISIBLE, false);
      dash_showing_animation_.Stop();
    }
    else if (identity == "hud")
    {
      hud_is_open_ = false;
    }

    // If they are both now shut, then disable the effect helper and saturate the icons.
    if (!IsOverlayOpen())
    {
      bg_effect_helper_.enabled = false;
      LOG_DEBUG(logger) << "Saturate on monitor " << monitor();
      SaturateIcons();
    }
  }

  // as the leave event is no more received when the place is opened
  // FIXME: remove when we change the mouse grab strategy in nux
  nux::Point pt = nux::GetWindowCompositor().GetMousePosition();
  SetStateMouseOverLauncher(GetAbsoluteGeometry().IsInside(pt));
}

bool Launcher::IsOverlayOpen() const
{
  auto& wm = WindowManager::Default();
  return dash_is_open_ || hud_is_open_ || wm.IsScaleActive() || wm.IsExpoActive();
}

void Launcher::SetHidden(bool hide_launcher)
{
  if (hide_launcher == hidden_)
    return;

  hidden_ = hide_launcher;
  hide_machine_.SetQuirk(LauncherHideMachine::LAUNCHER_HIDDEN, hide_launcher);
  hover_machine_.SetQuirk(LauncherHoverMachine::LAUNCHER_HIDDEN, hide_launcher);

  if (hide_launcher)
  {
    hide_machine_.SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, false);
    hide_machine_.SetQuirk(LauncherHideMachine::MT_DRAG_OUT, false);
    SetStateMouseOverLauncher(false);
  }

  animation::StartOrReverseIf(auto_hide_animation_, hide_launcher);

  postreveal_mousemove_delta_x_ = 0;
  postreveal_mousemove_delta_y_ = 0;

  if (nux::GetWindowThread()->IsEmbeddedWindow())
    parent_->EnableInputWindow(!hide_launcher, launcher::window_title, false, false);

  if (!hide_launcher && GetActionState() == ACTION_DRAG_EXTERNAL)
    DndReset();

  hidden_changed.emit();
}

void Launcher::UpdateChangeInMousePosition(int delta_x, int delta_y)
{
  postreveal_mousemove_delta_x_ += delta_x;
  postreveal_mousemove_delta_y_ += delta_y;

  // check the state before changing it to avoid uneeded hide calls
  if (!hide_machine_.GetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL) &&
     (std::abs(postreveal_mousemove_delta_x_) > MOUSE_DEADZONE ||
      std::abs(postreveal_mousemove_delta_y_) > MOUSE_DEADZONE))
  {
    hide_machine_.SetQuirk(LauncherHideMachine::MOUSE_MOVE_POST_REVEAL, true);
  }
}

int Launcher::GetMouseX() const
{
  return mouse_position_.x;
}

int Launcher::GetMouseY() const
{
  return mouse_position_.y;
}

void Launcher::OnPluginStateChanged()
{
  WindowManager& wm = WindowManager::Default();
  bool expo_active = wm.IsExpoActive();
  hide_machine_.SetQuirk(LauncherHideMachine::EXPO_ACTIVE, expo_active);
  // We must make sure that we regenerate the blur when expo is fully active
  // bg_effect_helper_.enabled = expo_active;

  if (expo_active)
  {
    if (!hovered_)
      DesaturateIcons();

    if (icon_under_mouse_)
      icon_under_mouse_->HideTooltip();
  }
  else
  {
    SaturateIcons();
  }
}

void Launcher::OnSpreadChanged()
{
  WindowManager& wm = WindowManager::Default();
  bool active = wm.IsScaleActive();
  hide_machine_.SetQuirk(LauncherHideMachine::SCALE_ACTIVE, active);

  bg_effect_helper_.enabled = active;

  if (active && icon_under_mouse_)
    icon_under_mouse_->HideTooltip();

  if (active && (!hovered_ || wm.IsScaleActiveForGroup()))
  {
    // The icons can take some ms to update their active state, this can protect us.
    sources_.AddIdle([this] { DesaturateIcons(); return false; }, SCALE_DESATURATE_IDLE);
  }
  else
  {
    sources_.Remove(SCALE_DESATURATE_IDLE);
    SaturateIcons();
  }
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
  icon_renderer_->monitor = new_monitor;
}

void Launcher::UpdateOptions(Options::Ptr options)
{
  SetIconSize(options->tile_size, options->icon_size);
  SetHideMode(options->hide_mode);

  if (model_)
  {
    for (auto const& icon : *model_)
      SetupIconAnimations(icon);
  }

  ConfigureBarrier();
  QueueDraw();
}

void Launcher::ConfigureBarrier()
{
  float decay_responsiveness_mult = ((options()->edge_responsiveness() - 1) * .3f) + 1;
  float reveal_responsiveness_mult = ((options()->edge_responsiveness() - 1) * .025f) + 1;

  hide_machine_.reveal_pressure = options()->edge_reveal_pressure() * reveal_responsiveness_mult;
  hide_machine_.edge_decay_rate = options()->edge_decay_rate() * decay_responsiveness_mult;
}

void Launcher::SetHideMode(LauncherHideMode hidemode)
{
  bool fixed_launcher = (hidemode == LAUNCHER_HIDE_NEVER);
  parent_->InputWindowEnableStruts(fixed_launcher);
  hide_machine_.SetMode(static_cast<LauncherHideMachine::HideMode>(hidemode));
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
  return active_tooltip_;
}

nux::ObjectPtr<LauncherDragWindow> const& Launcher::GetDraggedIcon() const
{
  return drag_window_;
}

void Launcher::SetActionState(LauncherActionState actionstate)
{
  if (launcher_action_state_ == actionstate)
    return;

  launcher_action_state_ = actionstate;

  hover_machine_.SetQuirk(LauncherHoverMachine::LAUNCHER_IN_ACTION, (actionstate != ACTION_NONE));
}

Launcher::LauncherActionState
Launcher::GetActionState() const
{
  return launcher_action_state_;
}

void Launcher::SetHover(bool hovered)
{
  if (hovered == hovered_)
    return;

  hovered_ = hovered;

  if (!IsInKeyNavMode() && hovered_)
    enter_y_ = mouse_position_.y;

  if (folded_)
    animation::StartOrReverseIf(hover_animation_, hovered_);

  if (IsOverlayOpen() && !hide_machine_.GetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE))
  {
    if (hovered && !hide_machine_.GetQuirk(LauncherHideMachine::SHORTCUT_KEYS_VISIBLE))
      SaturateIcons();
    else
      DesaturateIcons();
  }
}

bool Launcher::MouseOverTopScrollArea()
{
  return mouse_position_.y < SCROLL_AREA_HEIGHT;
}

bool Launcher::MouseOverBottomScrollArea()
{
  return mouse_position_.y >= GetGeometry().height - SCROLL_AREA_HEIGHT;
}

bool Launcher::OnScrollTimeout()
{
  bool continue_animation = true;

  if (IsInKeyNavMode() || !hovered_ || GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    continue_animation = false;
  }
  else if (MouseOverTopScrollArea())
  {
    if (launcher_drag_delta_ >= launcher_drag_delta_max_)
    {
      continue_animation = false;
    }
    else
    {
      int mouse_distance = (SCROLL_AREA_HEIGHT - mouse_position_.y);
      int speed = static_cast<float>(mouse_distance) / SCROLL_AREA_HEIGHT * SCROLL_FPS;
      launcher_drag_delta_ += speed;
    }
  }
  else if (MouseOverBottomScrollArea())
  {
    if (launcher_drag_delta_ <= launcher_drag_delta_min_)
    {
      continue_animation = false;
    }
    else
    {
      int mouse_distance = (mouse_position_.y + 1) - (GetGeometry().height - SCROLL_AREA_HEIGHT);
      int speed = static_cast<float>(mouse_distance) / SCROLL_AREA_HEIGHT * SCROLL_FPS;
      launcher_drag_delta_ -= speed;
    }
  }
  else
  {
    continue_animation = false;
  }

  if (continue_animation)
  {
    QueueDraw();
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

void Launcher::SetUrgentTimer(int urgent_animate_period)
{
  sources_.AddTimeoutSeconds(urgent_animate_period, sigc::mem_fun(this, &Launcher::OnUrgentTimeout), URGENT_TIMEOUT);
}

void Launcher::AnimateUrgentIcon(AbstractLauncherIcon::Ptr const& icon)
{
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, false, monitor());
  icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::URGENT, monitor());
  icon->SetQuirk(AbstractLauncherIcon::Quirk::URGENT, true, monitor());
}

void Launcher::HandleUrgentIcon(AbstractLauncherIcon::Ptr const& icon)
{
  if (!icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()))
  {
    if (animating_urgent_icons_.erase(icon) > 0)
    {
      if (animating_urgent_icons_.empty())
      {
        sources_.Remove(URGENT_TIMEOUT);
        sources_.Remove(LAST_ANIMATION_URGENT_IDLE);
      }
    }

    return;
  }

  bool animating = (animating_urgent_icons_.find(icon) != animating_urgent_icons_.end());

  if (hidden_ && !animating)
  {
    bool urgent_timer_running = sources_.GetSource(URGENT_TIMEOUT) != nullptr;

    // If the Launcher is hidden, then add a timer to wiggle the urgent icons at
    // certain intervals (1m, 2m, 4m, 8m, 16m, & 32m).
    if (!urgent_timer_running && !animating)
    {
      urgent_ack_needed_ = true;
      SetUrgentTimer(BASE_URGENT_ANIMATION_PERIOD);
    }
    // If the Launcher is hidden, the timer is running, an urgent icon is newer than the last time
    // icons were wiggled, and the timer did not just start, then reset the timer since a new
    // urgent icon just showed up.
    else if (urgent_timer_running && urgent_animation_period_ != 0)
    {
      urgent_animation_period_ = 0;
      SetUrgentTimer(BASE_URGENT_ANIMATION_PERIOD);
    }

    animating_urgent_icons_.insert(icon);
  }
  // If the Launcher is no longer hidden, then after the Launcher is fully revealed, wiggle the
  // urgent icon and then stop the timer (if it's running).
  else if (!hidden_ && urgent_ack_needed_)
  {
    sources_.AddIdle([this] {
      if (hidden_ || hide_machine_.reveal_progress == 1.0f)
        return false;

      if (hide_machine_.reveal_progress > 0)
        return true;

      for (auto const& icon : animating_urgent_icons_)
        AnimateUrgentIcon(icon);

      sources_.Remove(URGENT_TIMEOUT);
      urgent_ack_needed_ = false;
      return false;
    }, LAST_ANIMATION_URGENT_IDLE);
  }
}

bool Launcher::OnUrgentTimeout()
{
  bool foundUrgent = false;

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE && hidden_)
  {
    // Look for any icons that are still urgent and wiggle them
    for (auto icon : *model_)
    {
      if (icon->GetQuirk(AbstractLauncherIcon::Quirk::URGENT, monitor()))
      {
        AnimateUrgentIcon(icon);

        foundUrgent = true;
      }
    }
  }
  // Update the time for when the next wiggle will occur.
  if (urgent_animation_period_ == 0)
  {
    urgent_animation_period_ = BASE_URGENT_ANIMATION_PERIOD;
  }
  else
  {
    urgent_animation_period_ = urgent_animation_period_ * DOUBLE_TIME;
  }

  // If no urgent icons were found or we have reached the time threshold,
  // then let's stop the timer.  Otherwise, start a new timer with the
  // updated wiggle time.
  if (!foundUrgent || (urgent_animation_period_ > MAX_URGENT_ANIMATION_DELTA))
  {
    return false;
  }

  SetUrgentTimer(urgent_animation_period_);
  return false;
}

void Launcher::SetIconSize(int tile_size, int icon_size)
{
  ui::IconRenderer::DestroyShortcutTextures();

  icon_size_ = tile_size;
  icon_renderer_->SetTargetSize(icon_size_, icon_size, SPACE_BETWEEN_ICONS);
  AbstractLauncherIcon::icon_size = icon_size_;

  nux::Geometry const& parent_geo = parent_->GetGeometry();
  Resize(nux::Point(parent_geo.x, parent_geo.y), parent_geo.height);
}

int Launcher::GetIconSize() const
{
  return icon_size_;
}

void Launcher::Resize(nux::Point const& offset, int height)
{
  unsigned width = icon_size_ + ICON_PADDING * 2 + RIGHT_LINE_WIDTH - 2;
  SetMaximumHeight(height);
  SetGeometry(nux::Geometry(0, 0, width, height));
  parent_->SetGeometry(nux::Geometry(offset.x, offset.y, width, height));

  ConfigureBarrier();
}

void Launcher::OnIconNeedsRedraw(AbstractLauncherIcon::Ptr const& icon, int icon_monitor)
{
  if (icon_monitor < 0 || icon_monitor == monitor())
    QueueDraw();
}

void Launcher::OnIconAdded(AbstractLauncherIcon::Ptr const& icon)
{
  SetupIconAnimations(icon);

  icon->needs_redraw.connect(sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw));
  icon->tooltip_visible.connect(sigc::mem_fun(this, &Launcher::OnTooltipVisible));

  if (IsOverlayOpen() && !hovered_)
  {
    icon->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true, monitor());
    icon->SkipQuirkAnimation(AbstractLauncherIcon::Quirk::DESAT, monitor());
  }

  if (icon->IsVisibleOnMonitor(monitor()))
    QueueDraw();
}

void Launcher::OnIconRemoved(AbstractLauncherIcon::Ptr const& icon)
{
  SetIconUnderMouse(AbstractLauncherIcon::Ptr());
  if (icon == icon_mouse_down_)
    icon_mouse_down_ = nullptr;
  if (icon == drag_icon_)
    drag_icon_ = nullptr;

  animating_urgent_icons_.erase(icon);

  if (icon->IsVisibleOnMonitor(monitor()))
    QueueDraw();
}

void Launcher::SetupIconAnimations(AbstractLauncherIcon::Ptr const& icon)
{
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::VISIBLE, ANIM_DURATION_SHORT, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::RUNNING, ANIM_DURATION_SHORT, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::STARTING, (ANIM_DURATION_LONG * MAX_STARTING_BLINKS * STARTING_BLINK_LAMBDA * 2), monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::PULSE_ONCE, (ANIM_DURATION_LONG * PULSE_BLINK_LAMBDA * 2), monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::PRESENTED, ANIM_DURATION, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::UNFOLDED, ANIM_DURATION, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::SHIMMER, ANIM_DURATION_LONG, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::CENTER_SAVED, ANIM_DURATION, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::PROGRESS, ANIM_DURATION, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::DROP_DIM, ANIM_DURATION, monitor());
  icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::DESAT, ANIM_DURATION_SHORT_SHORT, monitor());

  if (options()->urgent_animation() == URGENT_ANIMATION_WIGGLE)
    icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::URGENT, (ANIM_DURATION_SHORT * WIGGLE_CYCLES), monitor());
  else
    icon->SetQuirkDuration(AbstractLauncherIcon::Quirk::URGENT, (ANIM_DURATION_LONG * URGENT_BLINKS * 2), monitor());
}

void Launcher::SetModel(LauncherModel::Ptr model)
{
  model_ = model;
  auto const& queue_draw_cb = sigc::mem_fun(this, &Launcher::OnIconNeedsRedraw);

  for (auto const& icon : *model_)
  {
    SetupIconAnimations(icon);
    icon->needs_redraw.connect(queue_draw_cb);
  }

  model_->icon_added.connect(sigc::mem_fun(this, &Launcher::OnIconAdded));
  model_->icon_removed.connect(sigc::mem_fun(this, &Launcher::OnIconRemoved));
  model_->order_changed.connect(sigc::mem_fun(this, &Launcher::QueueDraw));
  model_->selection_changed.connect(sigc::mem_fun(this, &Launcher::OnSelectionChanged));
}

LauncherModel::Ptr Launcher::GetModel() const
{
  return model_;
}

void Launcher::EnsureIconOnScreen(AbstractLauncherIcon::Ptr const& selection)
{
  nux::Geometry const& geo = GetGeometry();

  int natural_y = 0;
  for (auto icon : *model_)
  {
    if (!icon->IsVisibleOnMonitor(monitor))
      continue;

    if (icon == selection)
      break;

    natural_y += icon_size_ + SPACE_BETWEEN_ICONS;
  }

  int max_drag_delta = geo.height - (natural_y + icon_size_ + (2 * SPACE_BETWEEN_ICONS));
  int min_drag_delta = -natural_y;

  launcher_drag_delta_ = std::max<int>(min_drag_delta, std::min<int>(max_drag_delta, launcher_drag_delta_));
}

void Launcher::OnSelectionChanged(AbstractLauncherIcon::Ptr const& selection)
{
  if (IsInKeyNavMode())
  {
    EnsureIconOnScreen(selection);
    QueueDraw();
  }
}

void Launcher::OnTooltipVisible(nux::ObjectPtr<nux::View> view)
{
  active_tooltip_ = view;
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

  float reveal_progress = hide_machine_.reveal_progress;
  if ((reveal_progress > 0 || last_reveal_progress_ > 0) && launcher_pressure_effect_.IsValid())
  {
    if (std::abs(last_reveal_progress_ - reveal_progress) <= .1f)
    {
      last_reveal_progress_ = reveal_progress;
    }
    else
    {
      if (last_reveal_progress_ > reveal_progress)
        last_reveal_progress_ -= .1f;
      else
        last_reveal_progress_ += .1f;
    }
    nux::Color pressure_color = nux::color::White * last_reveal_progress_;
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
        {
          gPainter.PushDrawCompositionLayer(GfxContext, base,
                                            blur_texture,
                                            texxform_blur_bg,
                                            nux::color::White,
                                            options()->background_color, nux::LAYER_BLEND_MODE_OVERLAY,
                                            true, ROP);
        }
        else
        {
          gPainter.PushDrawTextureLayer(GfxContext, base,
                                        blur_texture,
                                        texxform_blur_bg,
                                        nux::color::White,
                                        true,
                                        ROP);
        }
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

  // XXX: It would be very cool to move the Rendering part out of the drawing part
  icon_renderer_->PreprocessIcons(args, base);
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

    icon_renderer_->RenderIcon(GfxContext, *rev_it, bkg_box, base);
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
  QueueDraw();
}

bool Launcher::StartIconDragTimeout(int x, int y)
{
  // if we are still waiting…
  if (GetActionState() == ACTION_NONE)
  {
    SetIconUnderMouse(AbstractLauncherIcon::Ptr());
    initial_drag_animation_ = true;
    StartIconDragRequest(x, y);
  }

  return false;
}

void Launcher::StartIconDragRequest(int x, int y)
{
  auto const& abs_geo = GetAbsoluteGeometry();
  auto const& drag_icon = MouseIconIntersection(abs_geo.width / 2.0f, y);

  // FIXME: nux doesn't give nux::GetEventButton (button_flags) there, relying
  // on an internal Launcher property then
  if (drag_icon && last_button_press_ == 1 && drag_icon->position() == AbstractLauncherIcon::Position::FLOATING)
  {
    auto const& icon_center = drag_icon->GetCenter(monitor);
    x += abs_geo.x;
    y += abs_geo.y;

    SetActionState(ACTION_DRAG_ICON);
    StartIconDrag(drag_icon);
    UpdateDragWindowPosition(icon_center.x, icon_center.y);

    if (initial_drag_animation_)
    {
      drag_window_->SetAnimationTarget(x, y);
      drag_window_->StartQuickAnimation();
    }

    QueueDraw();
  }
  else
  {
    drag_icon_ = nullptr;
    HideDragWindow();
  }
}

void Launcher::StartIconDrag(AbstractLauncherIcon::Ptr const& icon)
{
  using namespace std::placeholders;

  if (!icon)
    return;

  hide_machine_.SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, true);
  drag_icon_ = icon;
  drag_icon_position_ = model_->IconIndex(icon);

  HideDragWindow();

  auto cb = std::bind(&Launcher::RenderIconToTexture, this, _1, _2, drag_icon_);
  drag_window_ = new LauncherDragWindow(GetWidth(), cb);
  ShowDragWindow();
}

void Launcher::EndIconDrag()
{
  if (drag_window_)
  {
    AbstractLauncherIcon::Ptr hovered_icon;

    if (!drag_window_->Cancelled())
      hovered_icon = MouseIconIntersection(mouse_position_.x, mouse_position_.y);

    if (hovered_icon && hovered_icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH)
    {
      hovered_icon->SetQuirk(AbstractLauncherIcon::Quirk::PULSE_ONCE, true, monitor());

      remove_request.emit(drag_icon_);

      HideDragWindow();
      QueueDraw();
    }
    else
    {
      if (!drag_window_->Cancelled() && model_->IconIndex(drag_icon_) != drag_icon_position_)
      {
        drag_icon_->Stick(true);
      }

      auto const& icon_center = drag_icon_->GetCenter(monitor);
      drag_window_->SetAnimationTarget(icon_center.x, icon_center.y);
      drag_window_->anim_completed.connect(sigc::mem_fun(this, &Launcher::OnDragWindowAnimCompleted));
      drag_window_->StartQuickAnimation();
    }
  }

  if (MouseBeyondDragThreshold())
    animation::StartOrReverse(drag_icon_animation_, animation::Direction::FORWARD);

  hide_machine_.SetQuirk(LauncherHideMachine::INTERNAL_DND_ACTIVE, false);
}

void Launcher::ShowDragWindow()
{
  if (!drag_window_ || drag_window_->IsVisible())
    return;

  drag_window_->GrabKeyboard();
  drag_window_->ShowWindow(true);
  drag_window_->PushToFront();

  bool is_before;
  AbstractLauncherIcon::Ptr const& closer = model_->GetClosestIcon(drag_icon_, is_before);
  drag_window_->drag_cancel_request.connect([this, closer, is_before] {
    if (is_before)
      model_->ReorderAfter(drag_icon_, closer);
    else
      model_->ReorderBefore(drag_icon_, closer, true);

    ResetMouseDragState();
    SetActionState(ACTION_DRAG_ICON_CANCELLED);
  });
}

void Launcher::HideDragWindow()
{
  nux::Geometry const& abs_geo = GetAbsoluteGeometry();
  nux::Point const& mouse = nux::GetWindowCompositor().GetMousePosition();

  if (abs_geo.IsInside(mouse))
    mouse_enter.emit(mouse.x - abs_geo.x, mouse.y - abs_geo.y, 0, 0);

  if (!drag_window_)
    return;

  drag_window_->UnGrabKeyboard();
  drag_window_->ShowWindow(false);
  drag_window_ = nullptr;
}

void Launcher::UpdateDragWindowPosition(int x, int y)
{
  if (!drag_window_)
    return;

  auto const& icon_geo = drag_window_->GetGeometry();
  drag_window_->SetBaseXY(x - icon_geo.width / 2, y - icon_geo.height / 2);

  if (!drag_icon_)
    return;

  auto const& launcher_geo = GetGeometry();
  auto const& hovered_icon = MouseIconIntersection((launcher_geo.x + launcher_geo.width) / 2.0, y - GetAbsoluteY());
  bool mouse_beyond_drag_threshold = MouseBeyondDragThreshold();

  if (hovered_icon && drag_icon_ != hovered_icon)
  {
    if (!mouse_beyond_drag_threshold)
    {
      model_->ReorderSmart(drag_icon_, hovered_icon, true);
    }
    else
    {
      model_->ReorderBefore(drag_icon_, hovered_icon, false);
    }
  }
  else if (!hovered_icon && mouse_beyond_drag_threshold)
  {
    // If no icon is hovered, then we can add our icon to the bottom
    for (auto it = model_->main_rbegin(); it != model_->main_rend(); ++it)
    {
      auto const& icon = *it;

      if (!icon->IsVisibleOnMonitor(monitor))
        continue;

      if (y >= icon->GetCenter(monitor).y)
      {
        model_->ReorderAfter(drag_icon_, icon);
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
    hide_machine_.SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, false);

  SetActionState(ACTION_NONE);
  dnd_delta_x_ = 0;
  dnd_delta_y_ = 0;
  last_button_press_ = 0;
}

void Launcher::RecvMouseDown(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  last_button_press_ = nux::GetEventButton(button_flags);
  SetMousePosition(x, y);

  MouseDownLogic(x, y, button_flags, key_flags);
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

  if (last_button_press_ != 1)
    return;

  SetMousePosition(x, y);

  // FIXME: hack (see SetupRenderArg)
  initial_drag_animation_ = false;

  dnd_delta_y_ += dy;
  dnd_delta_x_ += dx;

  if (nux::Abs(dnd_delta_y_) < MOUSE_DEADZONE &&
      nux::Abs(dnd_delta_x_) < MOUSE_DEADZONE &&
      GetActionState() == ACTION_NONE)
    return;

  SetIconUnderMouse(AbstractLauncherIcon::Ptr());

  if (GetActionState() == ACTION_NONE)
  {
#ifdef USE_X11
    if (nux::Abs(dnd_delta_y_) >= nux::Abs(dnd_delta_x_))
    {
      launcher_drag_delta_ += dnd_delta_y_;
      SetActionState(ACTION_DRAG_LAUNCHER);
      hide_machine_.SetQuirk(LauncherHideMachine::VERTICAL_SLIDE_ACTIVE, true);
    }
    else
    {
      // We we can safely start the icon drag, from the original mouse-down position
      sources_.Remove(START_DRAGICON_DURATION);
      StartIconDragRequest(x - dnd_delta_x_, y - dnd_delta_y_);
    }
#endif
  }
  else if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    launcher_drag_delta_ += dy;
  }
  else if (GetActionState() == ACTION_DRAG_ICON)
  {
    nux::Geometry const& geo = GetAbsoluteGeometry();
    UpdateDragWindowPosition(geo.x + x, geo.y + y);
  }

  QueueDraw();
}

void Launcher::RecvMouseEnter(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);
  SetStateMouseOverLauncher(true);
  EventLogic();
}

void Launcher::RecvMouseLeave(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  SetStateMouseOverLauncher(false);
  EventLogic();
}

void Launcher::RecvMouseMove(int x, int y, int dx, int dy, unsigned long button_flags, unsigned long key_flags)
{
  SetMousePosition(x, y);

  if (!hidden_)
    UpdateChangeInMousePosition(dx, dy);

  // Every time the mouse moves, we check if it is inside an icon...
  EventLogic();

  if (icon_under_mouse_ && WindowManager::Default().IsScaleActiveForGroup())
  {
    if (!icon_under_mouse_->GetQuirk(AbstractLauncherIcon::Quirk::ACTIVE, monitor()))
      SaturateIcons();
  }

  tooltip_manager_.MouseMoved(icon_under_mouse_);
}

void Launcher::RecvMouseWheel(int /*x*/, int /*y*/, int wheel_delta, unsigned long /*button_flags*/, unsigned long key_flags)
{
  if (!hovered_)
    return;

  bool alt_pressed = nux::GetKeyModifierState(key_flags, nux::NUX_STATE_ALT);
  if (alt_pressed)
  {
    ScrollLauncher(wheel_delta);
  }
  else if (icon_under_mouse_)
  {
    auto timestamp = nux::GetGraphicsDisplay()->GetCurrentEvent().x11_timestamp;
    auto scroll_direction = (wheel_delta < 0) ? AbstractLauncherIcon::ScrollDirection::DOWN : AbstractLauncherIcon::ScrollDirection::UP;
    icon_under_mouse_->PerformScroll(scroll_direction, timestamp);
  }
}

void Launcher::ScrollLauncher(int wheel_delta)
{
  if (wheel_delta < 0)
    // scroll down
    launcher_drag_delta_ -= 25;
  else
    // scroll up
    launcher_drag_delta_ += 25;

  QueueDraw();
}

#ifdef USE_X11

ui::EdgeBarrierSubscriber::Result Launcher::HandleBarrierEvent(ui::PointerBarrierWrapper* owner, ui::BarrierEvent::Ptr event)
{
  if (hide_machine_.GetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE))
  {
    return ui::EdgeBarrierSubscriber::Result::NEEDS_RELEASE;
  }

  nux::Geometry const& abs_geo = GetAbsoluteGeometry();

  bool apply_to_reveal = false;
  if (event->x >= abs_geo.x && event->x <= abs_geo.x + abs_geo.width)
  {
    if (!hidden_)
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
    hide_machine_.AddRevealPressure(event->velocity);

  return ui::EdgeBarrierSubscriber::Result::HANDLED;
}

#endif

bool Launcher::IsInKeyNavMode() const
{
  return hide_machine_.GetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE);
}

void Launcher::EnterKeyNavMode()
{
  hide_machine_.SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, true);
  hover_machine_.SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, true);
  SaturateIcons();
}

void Launcher::ExitKeyNavMode()
{
  hide_machine_.SetQuirk(LauncherHideMachine::KEY_NAV_ACTIVE, false);
  hover_machine_.SetQuirk(LauncherHoverMachine::KEY_NAV_ACTIVE, false);
}

void Launcher::RecvQuicklistOpened(nux::ObjectPtr<QuicklistView> const& quicklist)
{
  UScreen* uscreen = UScreen::GetDefault();
  if (uscreen->GetMonitorGeometry(monitor).IsInside(nux::Point(quicklist->GetGeometry().x, quicklist->GetGeometry().y)))
  {
    hide_machine_.SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, true);
    hover_machine_.SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, true);
    EventLogic();
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

  hide_machine_.SetQuirk(LauncherHideMachine::QUICKLIST_OPEN, false);
  hover_machine_.SetQuirk(LauncherHoverMachine::QUICKLIST_OPEN, false);

  EventLogic();
}

void Launcher::EventLogic()
{
  if (GetActionState() == ACTION_DRAG_ICON ||
      GetActionState() == ACTION_DRAG_LAUNCHER)
    return;

  AbstractLauncherIcon::Ptr launcher_icon;

  if (!hidden_ && !IsInKeyNavMode() && hovered_)
  {
    launcher_icon = MouseIconIntersection(mouse_position_.x, mouse_position_.y);
  }

  SetIconUnderMouse(launcher_icon);
}

void Launcher::MouseDownLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon::Ptr const& launcher_icon = MouseIconIntersection(mouse_position_.x, mouse_position_.y);

  if (launcher_icon)
  {
    icon_mouse_down_ = launcher_icon;
    // if MouseUp after the time ended -> it's an icon drag, otherwise, it's starting an app
    auto cb_func = sigc::bind(sigc::mem_fun(this, &Launcher::StartIconDragTimeout), x, y);
    sources_.AddTimeout(START_DRAGICON_DURATION, cb_func, START_DRAGICON_TIMEOUT);

    launcher_icon->mouse_down.emit(nux::GetEventButton(button_flags), monitor, key_flags);
    tooltip_manager_.IconClicked();
  }
}

void Launcher::MouseUpLogic(int x, int y, unsigned long button_flags, unsigned long key_flags)
{
  AbstractLauncherIcon::Ptr const& launcher_icon = MouseIconIntersection(mouse_position_.x, mouse_position_.y);

  sources_.Remove(START_DRAGICON_TIMEOUT);

  if (icon_mouse_down_ && (icon_mouse_down_ == launcher_icon))
  {
    icon_mouse_down_->mouse_up.emit(nux::GetEventButton(button_flags), monitor, key_flags);

    if (GetActionState() == ACTION_NONE)
    {
      icon_mouse_down_->mouse_click.emit(nux::GetEventButton(button_flags), monitor, key_flags);
    }
  }

  if (launcher_icon && (icon_mouse_down_ != launcher_icon))
  {
    launcher_icon->mouse_up.emit(nux::GetEventButton(button_flags), monitor, key_flags);
  }

  if (GetActionState() == ACTION_DRAG_LAUNCHER)
  {
    animation::StartOrReverse(drag_over_animation_, animation::Direction::BACKWARD);
  }

  icon_mouse_down_ = nullptr;
}

AbstractLauncherIcon::Ptr Launcher::MouseIconIntersection(int x, int y) const
{
  LauncherModel::iterator it;
  // We are looking for the icon at screen coordinates x, y;
  nux::Point2 mouse_position(x, y);
  int inside = 0;

  for (it = model_->begin(); it != model_->end(); ++it)
  {
    if (!(*it)->IsVisibleOnMonitor(monitor))
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

void Launcher::RenderIconToTexture(nux::GraphicsEngine& GfxContext, nux::ObjectPtr<nux::IOpenGLBaseTexture> const& texture, AbstractLauncherIcon::Ptr const& icon)
{
  RenderArg arg;
  SetupRenderArg(icon, arg);
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

  GfxContext.QRP_Color(0, 0, texture->GetWidth(), texture->GetHeight(),
                       nux::color::Transparent);

  GfxContext.GetRenderStates().SetBlend(alpha, src, dest);

  nux::Geometry geo(0, 0, texture->GetWidth(), texture->GetWidth());

  icon_renderer_->PreprocessIcons(drag_args, geo);
  icon_renderer_->RenderIcon(GfxContext, arg, geo, geo);
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

  dnd_data_.Reset();
  drag_action_ = nux::DNDACTION_NONE;
  steal_drag_ = false;
  data_checked_ = false;
  dnd_hovered_icon_ = nullptr;
  drag_edge_touching_ = false;
  dnd_hide_animation_.Stop();
#endif
}

void Launcher::DndReset()
{
#ifdef USE_X11
  dnd_data_.Reset();

  bool is_overlay_open = IsOverlayOpen();

  for (auto it : *model_)
  {
    auto icon_type = it->GetIconType();
    bool desaturate = false;

    if (icon_type != AbstractLauncherIcon::IconType::HOME &&
        icon_type != AbstractLauncherIcon::IconType::HUD)
    {
      desaturate = is_overlay_open && !hovered_;
    }

    it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, desaturate, monitor());
    it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, false, monitor());
  }

  DndHoveredIconReset();
#endif
}

void Launcher::DndHoveredIconReset()
{
#ifdef USE_X11
  SetActionState(ACTION_NONE);

  if (steal_drag_ && dnd_hovered_icon_)
  {
    dnd_hovered_icon_->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false, monitor());
    dnd_hovered_icon_->remove.emit(dnd_hovered_icon_);
  }

  if (!steal_drag_ && dnd_hovered_icon_)
    dnd_hovered_icon_->SendDndLeave();

  steal_drag_ = false;
  drag_edge_touching_ = false;
  dnd_hovered_icon_ = nullptr;
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
  if (!data_checked_)
  {
    const std::string uri_list = "text/uri-list";
    data_checked_ = true;
    dnd_data_.Reset();
    auto& display = nux::GetWindowThread()->GetGraphicsDisplay();

    // get the data
    for (auto const& mime : mimes)
    {
      if (mime != uri_list)
        continue;

      dnd_data_.Fill(display.GetDndData(const_cast<char*>(uri_list.c_str())));
      break;
    }

    // see if the launcher wants this one
    auto const& uris = dnd_data_.Uris();
    if (std::find_if(uris.begin(), uris.end(), [this] (std::string const& uri)
                     {return DndIsSpecialRequest(uri);}) != uris.end())
    {
      steal_drag_ = true;
    }

    // only set hover once we know our first x/y
    SetActionState(ACTION_DRAG_EXTERNAL);
    SetStateMouseOverLauncher(true);
  }

  SetMousePosition(x - parent_->GetGeometry().x, y - parent_->GetGeometry().y);

  if (options()->hide_mode != LAUNCHER_HIDE_NEVER)
  {
    if (monitor() == 0 && !IsOverlayOpen() && mouse_position_.x == 0 && !drag_edge_touching_ &&
        mouse_position_.y <= (parent_->GetGeometry().height - icon_size_ - 2 * SPACE_BETWEEN_ICONS))
    {
      if (dnd_hovered_icon_)
          dnd_hovered_icon_->SendDndLeave();

      animation::StartOrReverse(dnd_hide_animation_, animation::Direction::FORWARD);
      drag_edge_touching_ = true;
    }
    else if (mouse_position_.x != 0 && drag_edge_touching_)
    {
      animation::StartOrReverse(dnd_hide_animation_, animation::Direction::BACKWARD);
      drag_edge_touching_ = false;
    }
  }

  EventLogic();
  auto const& hovered_icon = MouseIconIntersection(mouse_position_.x, mouse_position_.y);

  bool hovered_icon_is_appropriate = false;
  if (hovered_icon)
  {
    if (hovered_icon->GetIconType() == AbstractLauncherIcon::IconType::TRASH)
      steal_drag_ = false;

    if (hovered_icon->position() == AbstractLauncherIcon::Position::FLOATING)
      hovered_icon_is_appropriate = true;
  }

  if (steal_drag_)
  {
    drag_action_ = nux::DNDACTION_COPY;
    if (!dnd_hovered_icon_ && hovered_icon_is_appropriate)
    {
      dnd_hovered_icon_ = new SpacerLauncherIcon(monitor());
      model_->AddIcon(dnd_hovered_icon_);
      model_->ReorderBefore(dnd_hovered_icon_, hovered_icon, true);
    }
    else if (dnd_hovered_icon_)
    {
      if (hovered_icon)
      {
        if (hovered_icon_is_appropriate)
        {
          model_->ReorderSmart(dnd_hovered_icon_, hovered_icon, true);
        }
        else
        {
          dnd_hovered_icon_->SetQuirk(AbstractLauncherIcon::Quirk::VISIBLE, false, monitor());
          dnd_hovered_icon_->remove.emit(dnd_hovered_icon_);
          dnd_hovered_icon_ = nullptr;
        }
      }
    }
  }
  else
  {
    if (!drag_edge_touching_ && hovered_icon != dnd_hovered_icon_)
    {
      if (hovered_icon)
      {
        hovered_icon->SendDndEnter();
        drag_action_ = hovered_icon->QueryAcceptDrop(dnd_data_);
      }
      else
      {
        drag_action_ = nux::DNDACTION_NONE;
      }

      if (dnd_hovered_icon_)
        dnd_hovered_icon_->SendDndLeave();

      dnd_hovered_icon_ = hovered_icon;
    }
  }

  bool accept;
  if (drag_action_ != nux::DNDACTION_NONE)
    accept = true;
  else
    accept = false;

  SendDndStatus(accept, drag_action_, nux::Geometry(x, y, 1, 1));
#endif
}

void Launcher::ProcessDndDrop(int x, int y)
{
#ifdef USE_X11
  if (steal_drag_)
  {
    for (auto const& uri : dnd_data_.Uris())
    {
      if (DndIsSpecialRequest(uri))
        add_request.emit(uri, dnd_hovered_icon_);
    }
  }
  else if (dnd_hovered_icon_ && drag_action_ != nux::DNDACTION_NONE)
  {
     if (IsOverlayOpen())
       ubus_.SendMessage(UBUS_OVERLAY_CLOSE_REQUEST);

    dnd_hovered_icon_->AcceptDrop(dnd_data_);
  }

  if (drag_action_ != nux::DNDACTION_NONE)
    SendDndFinished(true, drag_action_);
  else
    SendDndFinished(false, drag_action_);

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

  return model_->Selection();
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
  return launcher_drag_delta_;
}

void Launcher::DndStarted(std::string const& data)
{
#ifdef USE_X11
  SetDndQuirk();

  dnd_data_.Fill(data.c_str());

  auto const& uris = dnd_data_.Uris();
  if (std::find_if(uris.begin(), uris.end(), [this] (std::string const& uri)
                   {return DndIsSpecialRequest(uri);}) != uris.end())
  {
    steal_drag_ = true;

    if (IsOverlayOpen())
      SaturateIcons();
  }
  else
  {
    for (auto const& it : *model_)
    {
      if (it->ShouldHighlightOnDrag(dnd_data_))
      {
        it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false, monitor());
        it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, true, monitor());
      }
      else
      {
        it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, true, monitor());
        it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, false, monitor());
      }
    }
  }
#endif
}

void Launcher::DndFinished()
{
#ifdef USE_X11
  UnsetDndQuirk();

  data_checked_ = false;

  DndReset();
#endif
}

void Launcher::SetDndQuirk()
{
#ifdef USE_X11
  hide_machine_.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, true);
#endif
}

void Launcher::UnsetDndQuirk()
{
#ifdef USE_X11

  if (IsOverlayOpen() && !hovered_)
  {
    DesaturateIcons();
  }
  else
  {
    for (auto const& it : *model_)
    {
      it->SetQuirk(AbstractLauncherIcon::Quirk::DESAT, false, monitor());
      it->SetQuirk(AbstractLauncherIcon::Quirk::UNFOLDED, false, monitor());
    }
  }

  hide_machine_.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
  hide_machine_.SetQuirk(LauncherHideMachine::EXTERNAL_DND_ACTIVE, false);
#endif
}

} // namespace launcher
} // namespace unity
