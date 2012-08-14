/*
 * GesturalWindowSwitcher.cpp
 * This file is part of Unity
 *
 * Copyright (C) 2012 - Canonical Ltd.
 *
 * Unity is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Unity is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "GesturalWindowSwitcher.h"
#include <Nux/Nux.h>
#include "unityshell.h"

using namespace nux;
using namespace unity;

const float GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION = 100.0f;
const float GesturalWindowSwitcher::MOUSE_DRAG_THRESHOLD = 20.0f;

GesturalWindowSwitcher::GesturalWindowSwitcher()
{
  state_ = State::WaitingCompoundGesture;

  unity_screen_ = unity::UnityScreen::get(screen);
  switcher_controller_ = unity_screen_->switcher_controller();

  timer_close_switcher_.setCallback(
      boost::bind(&GesturalWindowSwitcher::OnCloseSwitcherTimeout, this));

  view_built_connection_ = switcher_controller_->view_built.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcher::ConnectToSwitcherViewMouseEvents));
}

GesturalWindowSwitcher::~GesturalWindowSwitcher()
{
  view_built_connection_.disconnect();
  mouse_down_connection_.disconnect();
  mouse_up_connection_.disconnect();
  mouse_drag_connection_.disconnect();
}

GestureDeliveryRequest GesturalWindowSwitcher::GestureEvent(const nux::GestureEvent &event)
{
  switch (state_)
  {
    case State::WaitingCompoundGesture:
      return WaitingCompoundGesture(event);
      break;
    case State::WaitingEndOfTapAndHold:
      return WaitingEndOfTapAndHold(event);
      break;
    case State::WaitingSwitcherManipulation:
      return WaitingSwitcherManipulation(event);
      break;
    case State::DraggingSwitcher:
      return DraggingSwitcher(event);
      break;
    case State::RecognizingMouseClickOrDrag:
      return RecognizingMouseClickOrDrag(event);
      break;
    case State::DraggingSwitcherWithMouse:
      return DraggingSwitcherWithMouse(event);
      break;
    case State::WaitingMandatorySwitcherClose:
      // do nothing
      return GestureDeliveryRequest::NONE;
      break;
    default:
      g_assert(false); // should never happen
      return GestureDeliveryRequest::NONE;
      break;
  }
}

GestureDeliveryRequest GesturalWindowSwitcher::WaitingCompoundGesture(const nux::GestureEvent &event)
{
  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  switch(gesture_recognizer_.GestureEvent(event))
  {
    case RecognitionResult::NONE:
      // Do nothing;
      break;
    case RecognitionResult::DOUBLE_TAP_RECOGNIZED:
      InitiateSwitcherNext();
      CloseSwitcherAfterTimeout(SWITCHER_TIME_AFTER_DOUBLE_TAP);
      break;
    default: // RecognitionResult::TAP_AND_HOLD_RECOGNIZED:
      InitiateSwitcherNext();
      request = GestureDeliveryRequest::EXCLUSIVITY;
      state_ = State::WaitingEndOfTapAndHold;
  }

  return request;
}

GestureDeliveryRequest GesturalWindowSwitcher::WaitingEndOfTapAndHold(const nux::GestureEvent &event)
{
  // There should be no simultaneous/overlapping gestures.
  g_assert(event.type != EVENT_GESTURE_BEGIN);

  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  if (event.type == EVENT_GESTURE_UPDATE)
  {
    if (event.GetGestureClasses() & nux::DRAG_GESTURE)
    {
      state_ = State::DraggingSwitcher;
      accumulated_horizontal_drag_ = 0.0f;
      request = DraggingSwitcher(event);
    }
  }
  else // event.type == EVENT_GESTURE_END
  {
    CloseSwitcherAfterTimeout(SWITCHER_TIME_AFTER_HOLD_RELEASED);
    state_ = State::WaitingSwitcherManipulation;
  }

  return request;
}

GestureDeliveryRequest
GesturalWindowSwitcher::WaitingSwitcherManipulation(const nux::GestureEvent &event)
{
  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  if (event.type == EVENT_GESTURE_BEGIN)
  {
    // Don't leak gestures to windows behind the switcher
    request = GestureDeliveryRequest::EXCLUSIVITY;
  }

  if (event.GetGestureClasses() & nux::DRAG_GESTURE)
  {
    state_ = State::DraggingSwitcher;
    timer_close_switcher_.stop();
    DraggingSwitcher(event);
  }

  return request;
}

GestureDeliveryRequest GesturalWindowSwitcher::DraggingSwitcher(const nux::GestureEvent &event)
{
  // There should be no simultaneous/overlapping gestures.
  g_assert(event.type != EVENT_GESTURE_BEGIN);

  g_assert(event.GetGestureClasses() & nux::DRAG_GESTURE);

  if (event.type == EVENT_GESTURE_UPDATE)
  {
    accumulated_horizontal_drag_ += event.GetDelta().x;
    ProcessAccumulatedHorizontalDrag();
  }
  else // event.type == EVENT_GESTURE_END
  {
    CloseSwitcher();
    state_ = State::WaitingCompoundGesture;
  }

  return GestureDeliveryRequest::NONE;
}

GestureDeliveryRequest
GesturalWindowSwitcher::RecognizingMouseClickOrDrag(const nux::GestureEvent &event)
{
  // Mouse press event has come but gestures have precedence over mouse
  // for interacting with the switcher.
  // Therefore act in the same way is if the mouse press didn't come
  return WaitingSwitcherManipulation(event);
}

GestureDeliveryRequest
GesturalWindowSwitcher::DraggingSwitcherWithMouse(const nux::GestureEvent &event)
{
  // Mouse press event has come but gestures have precedence over mouse
  // for interacting with the switcher.
  // Therefore act in the same way is if the mouse press didn't come
  return WaitingSwitcherManipulation(event);
}

void GesturalWindowSwitcher::CloseSwitcherAfterTimeout(int timeout)
{
  timer_close_switcher_.stop();
  // min and max timeouts
  timer_close_switcher_.setTimes(timeout,
                                 timeout+50);
  timer_close_switcher_.start();
}

bool GesturalWindowSwitcher::OnCloseSwitcherTimeout()
{

  switch (state_)
  {
    case State::WaitingSwitcherManipulation:
    case State::WaitingMandatorySwitcherClose:
      state_ = State::WaitingCompoundGesture;
      CloseSwitcher();
      break;
    default:
      CloseSwitcher();
  }
  // I'm assuming that by returning false I'm telling the timer to stop.
  return false;
}

void GesturalWindowSwitcher::CloseSwitcher()
{
  switcher_controller_->Hide();
}

void GesturalWindowSwitcher::InitiateSwitcherNext()
{
  timer_close_switcher_.stop();

  if (switcher_controller_->Visible())
    switcher_controller_->Next();
  else
  {
    unity_screen_->SetUpAndShowSwitcher();
  }
}

void GesturalWindowSwitcher::InitiateSwitcherPrevious()
{
  timer_close_switcher_.stop();

  if (switcher_controller_->Visible())
  {
    switcher_controller_->Prev();
  }
}

void GesturalWindowSwitcher::ProcessSwitcherViewMouseDown(int x, int y,
    unsigned long button_flags, unsigned long key_flags)
{
  if (state_ != State::WaitingSwitcherManipulation)
    return;

  // Don't close the switcher while the mouse is pressed over it
  timer_close_switcher_.stop();

  state_ = State::RecognizingMouseClickOrDrag;

  index_icon_hit_ = switcher_controller_->GetView()->IconIndexAt(x, y);
  accumulated_horizontal_drag_ = 0.0f;
}

void GesturalWindowSwitcher::ProcessSwitcherViewMouseUp(int x, int y,
    unsigned long button_flags, unsigned long key_flags)
{
  switch (state_)
  {
    case State::RecognizingMouseClickOrDrag:
      if (index_icon_hit_ >= 0)
      {
        // it was a click after all.
        switcher_controller_->Select(index_icon_hit_);
        // it was not a double tap gesture but we use the same timeout
        CloseSwitcherAfterTimeout(SWITCHER_TIME_AFTER_DOUBLE_TAP);
        state_ = State::WaitingMandatorySwitcherClose;
      }
      else
      {
        CloseSwitcher();
        state_ = State::WaitingCompoundGesture;
      }
      break;
    case State::DraggingSwitcherWithMouse:
      CloseSwitcher();
      state_ = State::WaitingCompoundGesture;
      break;
    default:
      // do nothing
      break;
  }
}

void GesturalWindowSwitcher::ProcessSwitcherViewMouseDrag(int x, int y, int dx, int dy,
    unsigned long button_flags, unsigned long key_flags)
{
  switch (state_)
  {
    case State::RecognizingMouseClickOrDrag:
      accumulated_horizontal_drag_ += dx;
      if (fabsf(accumulated_horizontal_drag_) >= MOUSE_DRAG_THRESHOLD)
      {
        state_ = State::DraggingSwitcherWithMouse;
        ProcessAccumulatedHorizontalDrag();
      }
      break;
    case State::DraggingSwitcherWithMouse:
      accumulated_horizontal_drag_ += dx;
      ProcessAccumulatedHorizontalDrag();
      break;
    default:
      // do nothing
      break;
  }
}

void GesturalWindowSwitcher::ProcessAccumulatedHorizontalDrag()
{
  if (accumulated_horizontal_drag_ >= DRAG_DELTA_FOR_CHANGING_SELECTION)
  {
    InitiateSwitcherNext();
    accumulated_horizontal_drag_ = 0.0f;
  }
  else if (accumulated_horizontal_drag_ <= -DRAG_DELTA_FOR_CHANGING_SELECTION)
  {
    InitiateSwitcherPrevious();
    accumulated_horizontal_drag_ = 0.0f;
  }
}

void GesturalWindowSwitcher::ConnectToSwitcherViewMouseEvents()
{
  unity::switcher::SwitcherView *switcher_view = switcher_controller_->GetView();
  g_assert(switcher_view);

  mouse_down_connection_ = switcher_view->mouse_down.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcher::ProcessSwitcherViewMouseDown));

  mouse_up_connection_ = switcher_view->mouse_up.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcher::ProcessSwitcherViewMouseUp));

  mouse_drag_connection_ = switcher_view->mouse_drag.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcher::ProcessSwitcherViewMouseDrag));
}
