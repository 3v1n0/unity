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
#include <core/timer.h>
#include <Nux/Nux.h>
#include <NuxCore/Logger.h>
#include <UnityCore/ConnectionManager.h>
#include "SwitcherView.h"
#include "unityshell.h"

DECLARE_LOGGER(logger, "unity.gesture.switcher");

using namespace nux;
using namespace unity;

const float GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION = 100.0f;
const float GesturalWindowSwitcher::MOUSE_DRAG_THRESHOLD = 20.0f;

namespace unity
{
  class GesturalWindowSwitcherPrivate
  {
   public:
    GesturalWindowSwitcherPrivate();

    void CloseSwitcherAfterTimeout(int timeout);
    bool OnCloseSwitcherTimeout();
    void CloseSwitcher();

    // show the switcher and select the next application/window
    void InitiateSwitcherNext();

    // show the switcher and select the previous application/window
    void InitiateSwitcherPrevious();

    void ProcessAccumulatedHorizontalDrag();

    nux::GestureDeliveryRequest GestureEvent(nux::GestureEvent const& event);

    nux::GestureDeliveryRequest WaitingCompoundGesture(nux::GestureEvent const& event);
    nux::GestureDeliveryRequest WaitingEndOfTapAndHold(nux::GestureEvent const& event);
    nux::GestureDeliveryRequest WaitingSwitcherManipulation(nux::GestureEvent const& event);
    nux::GestureDeliveryRequest DraggingSwitcher(nux::GestureEvent const& event);
    nux::GestureDeliveryRequest RecognizingMouseClickOrDrag(nux::GestureEvent const& event);
    nux::GestureDeliveryRequest DraggingSwitcherWithMouse(nux::GestureEvent const& event);

    void ProcessSwitcherViewMouseDown(int x, int y,
        unsigned long button_flags, unsigned long key_flags);
    void ProcessSwitcherViewMouseUp(int x, int y,
        unsigned long button_flags, unsigned long key_flags);
    void ProcessSwitcherViewMouseDrag(int x, int y, int dx, int dy,
        unsigned long button_flags, unsigned long key_flags);

    void ConnectToSwitcherViewMouseEvents();

    enum class State
    {
      WaitingCompoundGesture,
      WaitingEndOfTapAndHold,
      WaitingSwitcherManipulation,
      DraggingSwitcher,
      RecognizingMouseClickOrDrag,
      DraggingSwitcherWithMouse,
      WaitingMandatorySwitcherClose,
    } state;

    unity::UnityScreen* unity_screen;
    unity::switcher::Controller::Ptr switcher_controller;
    CompoundGestureRecognizer gesture_recognizer;
    CompTimer timer_close_switcher;
    float accumulated_horizontal_drag;
    int index_icon_hit;

    connection::Manager connections_;
  };
}

///////////////////////////////////////////
// private class

GesturalWindowSwitcherPrivate::GesturalWindowSwitcherPrivate()
  : accumulated_horizontal_drag(0.0f)
{
  state = State::WaitingCompoundGesture;

  unity_screen = unity::UnityScreen::get(screen);
  switcher_controller = unity_screen->switcher_controller();

  timer_close_switcher.setCallback(
      boost::bind(&GesturalWindowSwitcherPrivate::OnCloseSwitcherTimeout, this));

  connections_.Add(switcher_controller->ConnectToViewBuilt(
        sigc::mem_fun(this, &GesturalWindowSwitcherPrivate::ConnectToSwitcherViewMouseEvents)));
}

GestureDeliveryRequest GesturalWindowSwitcherPrivate::GestureEvent(nux::GestureEvent const& event)
{
  switch (state)
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

GestureDeliveryRequest GesturalWindowSwitcherPrivate::WaitingCompoundGesture(nux::GestureEvent const& event)
{
  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  switch (gesture_recognizer.GestureEvent(event))
  {
    case RecognitionResult::NONE:
      // Do nothing;
      break;
    case RecognitionResult::DOUBLE_TAP_RECOGNIZED:
      InitiateSwitcherNext();
      CloseSwitcherAfterTimeout(GesturalWindowSwitcher::SWITCHER_TIME_AFTER_DOUBLE_TAP);
      break;
    default: // RecognitionResult::TAP_AND_HOLD_RECOGNIZED:
      InitiateSwitcherNext();
      request = GestureDeliveryRequest::EXCLUSIVITY;
      state = State::WaitingEndOfTapAndHold;
  }

  return request;
}

GestureDeliveryRequest GesturalWindowSwitcherPrivate::WaitingEndOfTapAndHold(nux::GestureEvent const& event)
{
  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  if (event.type == EVENT_GESTURE_BEGIN)
  {
    LOG_ERROR(logger) << "There should be no simultaneous/overlapping gestures.";
    return request;
  }

  if (event.type == EVENT_GESTURE_UPDATE)
  {
    if (event.GetGestureClasses() & nux::DRAG_GESTURE)
    {
      state = State::DraggingSwitcher;
      accumulated_horizontal_drag = 0.0f;
      request = DraggingSwitcher(event);
    }
  }
  else // event.type == EVENT_GESTURE_END
  {
    CloseSwitcherAfterTimeout(GesturalWindowSwitcher::SWITCHER_TIME_AFTER_HOLD_RELEASED);
    state = State::WaitingSwitcherManipulation;
  }

  return request;
}

GestureDeliveryRequest
GesturalWindowSwitcherPrivate::WaitingSwitcherManipulation(nux::GestureEvent const& event)
{
  GestureDeliveryRequest request = GestureDeliveryRequest::NONE;

  if (event.type == EVENT_GESTURE_BEGIN)
  {
    // Don't leak gestures to windows behind the switcher
    request = GestureDeliveryRequest::EXCLUSIVITY;
  }

  if (event.GetGestureClasses() & nux::DRAG_GESTURE)
  {
    state = State::DraggingSwitcher;
    timer_close_switcher.stop();
    DraggingSwitcher(event);
  }

  return request;
}

GestureDeliveryRequest GesturalWindowSwitcherPrivate::DraggingSwitcher(nux::GestureEvent const& event)
{
  if (event.type == EVENT_GESTURE_BEGIN)
  {
    LOG_ERROR(logger) << "There should be no simultaneous/overlapping gestures.";
    return GestureDeliveryRequest::NONE;
  }

  if (!(event.GetGestureClasses() & nux::DRAG_GESTURE))
  {
    LOG_ERROR(logger) << "Didn't get the expected drag gesture.";
    return GestureDeliveryRequest::NONE;
  }

  if (event.type == EVENT_GESTURE_UPDATE)
  {
    accumulated_horizontal_drag += event.GetDelta().x;
    ProcessAccumulatedHorizontalDrag();
  }
  else // event.type == EVENT_GESTURE_END
  {
    CloseSwitcher();
    state = State::WaitingCompoundGesture;
  }

  return GestureDeliveryRequest::NONE;
}

GestureDeliveryRequest
GesturalWindowSwitcherPrivate::RecognizingMouseClickOrDrag(nux::GestureEvent const& event)
{
  // Mouse press event has come but gestures have precedence over mouse
  // for interacting with the switcher.
  // Therefore act in the same way is if the mouse press didn't come
  return WaitingSwitcherManipulation(event);
}

GestureDeliveryRequest
GesturalWindowSwitcherPrivate::DraggingSwitcherWithMouse(nux::GestureEvent const& event)
{
  // Mouse press event has come but gestures have precedence over mouse
  // for interacting with the switcher.
  // Therefore act in the same way is if the mouse press didn't come
  return WaitingSwitcherManipulation(event);
}

void GesturalWindowSwitcherPrivate::CloseSwitcherAfterTimeout(int timeout)
{
  timer_close_switcher.stop();
  // min and max timeouts
  timer_close_switcher.setTimes(timeout,
                                 timeout+50);
  timer_close_switcher.start();
}

bool GesturalWindowSwitcherPrivate::OnCloseSwitcherTimeout()
{

  switch (state)
  {
    case State::WaitingSwitcherManipulation:
    case State::WaitingMandatorySwitcherClose:
      state = State::WaitingCompoundGesture;
      CloseSwitcher();
      break;
    default:
      CloseSwitcher();
  }
  // I'm assuming that by returning false I'm telling the timer to stop.
  return false;
}

void GesturalWindowSwitcherPrivate::CloseSwitcher()
{
  switcher_controller->Hide();
}

void GesturalWindowSwitcherPrivate::InitiateSwitcherNext()
{
  timer_close_switcher.stop();

  if (switcher_controller->Visible())
    switcher_controller->Next();
  else
  {
    unity_screen->SetUpAndShowSwitcher();
  }
}

void GesturalWindowSwitcherPrivate::InitiateSwitcherPrevious()
{
  timer_close_switcher.stop();

  if (switcher_controller->Visible())
  {
    switcher_controller->Prev();
  }
}

void GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseDown(int x, int y,
    unsigned long button_flags, unsigned long key_flags)
{
  if (state != State::WaitingSwitcherManipulation)
    return;

  // Don't close the switcher while the mouse is pressed over it
  timer_close_switcher.stop();

  state = State::RecognizingMouseClickOrDrag;

  auto view = switcher_controller->GetView();

  index_icon_hit = view->IconIndexAt(x, y);
  accumulated_horizontal_drag = 0.0f;
}

void GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseUp(int x, int y,
    unsigned long button_flags, unsigned long key_flags)
{
  switch (state)
  {
    case State::RecognizingMouseClickOrDrag:
      if (index_icon_hit >= 0)
      {
        // it was a click after all.
        switcher_controller->Select(index_icon_hit);
        // it was not a double tap gesture but we use the same timeout
        CloseSwitcherAfterTimeout(GesturalWindowSwitcher::SWITCHER_TIME_AFTER_DOUBLE_TAP);
        state = State::WaitingMandatorySwitcherClose;
      }
      else
      {
        CloseSwitcher();
        state = State::WaitingCompoundGesture;
      }
      break;
    case State::DraggingSwitcherWithMouse:
      CloseSwitcher();
      state = State::WaitingCompoundGesture;
      break;
    default:
      // do nothing
      break;
  }
}

void GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseDrag(int x, int y, int dx, int dy,
    unsigned long button_flags, unsigned long key_flags)
{
  switch (state)
  {
    case State::RecognizingMouseClickOrDrag:
      accumulated_horizontal_drag += dx;
      if (fabsf(accumulated_horizontal_drag) >=
          GesturalWindowSwitcher::MOUSE_DRAG_THRESHOLD)
      {
        state = State::DraggingSwitcherWithMouse;
        ProcessAccumulatedHorizontalDrag();
      }
      break;
    case State::DraggingSwitcherWithMouse:
      accumulated_horizontal_drag += dx;
      ProcessAccumulatedHorizontalDrag();
      break;
    default:
      // do nothing
      break;
  }
}

void GesturalWindowSwitcherPrivate::ProcessAccumulatedHorizontalDrag()
{
  if (accumulated_horizontal_drag >=
      GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION)
  {
    InitiateSwitcherNext();
    accumulated_horizontal_drag = 0.0f;
  }
  else if (accumulated_horizontal_drag <=
      -GesturalWindowSwitcher::DRAG_DELTA_FOR_CHANGING_SELECTION)
  {
    InitiateSwitcherPrevious();
    accumulated_horizontal_drag = 0.0f;
  }
}

void GesturalWindowSwitcherPrivate::ConnectToSwitcherViewMouseEvents()
{
  auto switcher_view = switcher_controller->GetView();
  g_assert(switcher_view);

  connections_.Add(switcher_view->mouse_down.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseDown)));

  connections_.Add(switcher_view->mouse_up.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseUp)));

  connections_.Add(switcher_view->mouse_drag.connect(
      sigc::mem_fun(this, &GesturalWindowSwitcherPrivate::ProcessSwitcherViewMouseDrag)));
}

///////////////////////////////////////////
// public class

GesturalWindowSwitcher::GesturalWindowSwitcher()
  : p(new GesturalWindowSwitcherPrivate)
{
}

GesturalWindowSwitcher::~GesturalWindowSwitcher()
{
  delete p;
}

GestureDeliveryRequest GesturalWindowSwitcher::GestureEvent(nux::GestureEvent const& event)
{
  return p->GestureEvent(event);
}
