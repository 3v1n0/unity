/*
 * GesturalWindowSwitcher.h
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
 * along with Unity; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef GESTURAL_WINDOW_SWITCHER_H
#define GESTURAL_WINDOW_SWITCHER_H

#include <Nux/Gesture.h>
#include "CompoundGestureRecognizer.h"
#include "SwitcherController.h"
#include <core/timer.h>

namespace unity
{
  class UnityScreen;
}

class GesturalWindowSwitcher : public nux::GestureTarget
{
  public:
    GesturalWindowSwitcher();
    virtual ~GesturalWindowSwitcher();

    // in milliseconds
    static const int SWITCHER_TIME_AFTER_DOUBLE_TAP = 350;
    static const int SWITCHER_TIME_AFTER_HOLD_RELEASED = 7000;

    // How far, in screen pixels, a drag gesture must go in order
    // to trigger a change in the selected window.
    static const float DRAG_DELTA_FOR_CHANGING_SELECTION;

    // How far, in screen pixels, a mouse pointer must move in order
    // to be considered dragging the switcher.
    static const float MOUSE_DRAG_THRESHOLD;

    virtual nux::GestureDeliveryRequest GestureEvent(const nux::GestureEvent &event);

  private:
    void CloseSwitcherAfterTimeout(int timeout);
    bool OnCloseSwitcherTimeout();
    void CloseSwitcher();

    // show the switcher and select the next application/window
    void InitiateSwitcherNext();

    // show the switcher and select the previous application/window
    void InitiateSwitcherPrevious();

    void ProcessAccumulatedHorizontalDrag();

    nux::GestureDeliveryRequest WaitingCompoundGesture(const nux::GestureEvent &event);
    nux::GestureDeliveryRequest WaitingEndOfTapAndHold(const nux::GestureEvent &event);
    nux::GestureDeliveryRequest WaitingSwitcherManipulation(const nux::GestureEvent &event);
    nux::GestureDeliveryRequest DraggingSwitcher(const nux::GestureEvent &event);
    nux::GestureDeliveryRequest RecognizingMouseClickOrDrag(const nux::GestureEvent &event);
    nux::GestureDeliveryRequest DraggingSwitcherWithMouse(const nux::GestureEvent &event);

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
    } state_;

    unity::UnityScreen *unity_screen_;
    unity::switcher::Controller::Ptr switcher_controller_;
    CompoundGestureRecognizer gesture_recognizer_;
    CompTimer timer_close_switcher_;
    float accumulated_horizontal_drag_;
    int index_icon_hit_;

    sigc::connection view_built_connection_;
    sigc::connection mouse_down_connection_;
    sigc::connection mouse_up_connection_;
    sigc::connection mouse_drag_connection_;
};
typedef std::shared_ptr<GesturalWindowSwitcher> ShPtGesturalWindowSwitcher;

#endif // GESTURAL_WINDOW_SWITCHER_H
