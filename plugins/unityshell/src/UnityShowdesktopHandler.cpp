// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/* Compiz unity plugin
 * unity.h
 *
 * Copyright (c) 2010-11 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Your own copyright notice would go above. You are free to choose whatever
 * licence you want, just take note that some compiz code is GPL and you will
 * not be able to re-use it if you want to use a different licence.
 */

#include <glib.h>
#include "UnityShowdesktopHandler.h"
#include "unity-shared/UnitySettings.h"

namespace unity
{

ShowdesktopHandlerWindowInterface::~ShowdesktopHandlerWindowInterface()
{
}

/* 300 ms */
const unsigned int ShowdesktopHandler::fade_time = 300;
std::list <ShowdesktopHandlerWindowInterface *> ShowdesktopHandler::animating_windows (0);

bool ShowdesktopHandler::ShouldHide (ShowdesktopHandlerWindowInterface *wi)
{
  if (wi->OverrideRedirect())
    return false;

  if (!wi->Managed())
    return false;

  if (wi->Grabbed())
    return false;

  if (wi->DesktopOrDock())
   return false;

  if (wi->SkipTaskbarOrPager())
    return false;

  if (wi->Hidden())
    if ((wi->ShowDesktopMode() || wi->Shaded() || wi->Minimized()))
      return false;

  return true;
}

guint32 ShowdesktopHandler::inhibiting_xid = 0;

void
ShowdesktopHandler::InhibitLeaveShowdesktopMode (guint32 xid)
{
  if (!inhibiting_xid)
    inhibiting_xid = xid;
}

void
ShowdesktopHandler::AllowLeaveShowdesktopMode (guint32 xid)
{
  if (inhibiting_xid == xid)
    inhibiting_xid = 0;
}

guint32
ShowdesktopHandler::InhibitingXid()
{
  return inhibiting_xid;
}

ShowdesktopHandler::ShowdesktopHandler (ShowdesktopHandlerWindowInterface *wi, compiz::WindowInputRemoverLockAcquireInterface *lock_acquire_interface)
  : showdesktop_handler_window_interface_(wi)
  , lock_acquire_interface_(lock_acquire_interface)
  , remover_()
  , state_(StateVisible)
  , progress_(0.0f)
  , was_hidden_(false)
{
}

ShowdesktopHandler::~ShowdesktopHandler()
{
}

void ShowdesktopHandler::FadeOut()
{
  if (state_ != StateVisible && state_ != StateFadeIn)
    return;

  state_ = ShowdesktopHandler::StateFadeOut;
  progress_ = Settings::Instance().low_gfx() ? 1.0f : 0.0f;

  was_hidden_ = showdesktop_handler_window_interface_->Hidden();

  if (!was_hidden_)
  {
    showdesktop_handler_window_interface_->Hide();
    showdesktop_handler_window_interface_->NotifyHidden();
    remover_ = lock_acquire_interface_->InputRemover();

    if (std::find (animating_windows.begin(),
                   animating_windows.end(),
		   showdesktop_handler_window_interface_) == animating_windows.end())
      animating_windows.push_back (showdesktop_handler_window_interface_);

  }
}

void ShowdesktopHandler::FadeIn()
{
  if (state_ != StateInvisible && state_ != StateFadeOut)
    return;

  state_ = ShowdesktopHandler::StateFadeIn;

  if (!was_hidden_)
  {
    showdesktop_handler_window_interface_->Show();
    showdesktop_handler_window_interface_->NotifyShown();
    remover_.reset();

    if (std::find (animating_windows.begin(),
                   animating_windows.end(),
		   showdesktop_handler_window_interface_) == animating_windows.end())
      animating_windows.push_back(showdesktop_handler_window_interface_);
  }
}

ShowdesktopHandlerWindowInterface::PostPaintAction ShowdesktopHandler::Animate (unsigned int ms)
{
  float inc = ms / static_cast <float> (fade_time);

  if (state_ == ShowdesktopHandler::StateFadeOut)
  {
    progress_ = Settings::Instance().low_gfx() ? 1.0f : progress_ + inc;
    if (progress_ >= 1.0f)
    {
      progress_ = 1.0f;
      state_ = StateInvisible;
    }
  }
  else if (state_ == StateFadeIn)
  {
    progress_ = Settings::Instance().low_gfx() ? 0.0f : progress_ - inc;
    if (progress_ <= 0.0f)
    {
      progress_ = 0.0f;
      state_ = StateVisible;
    }
  }
  else if (state_ == StateVisible)
    return ShowdesktopHandlerWindowInterface::PostPaintAction::Remove;
  else if (state_ == StateInvisible)
    return ShowdesktopHandlerWindowInterface::PostPaintAction::Wait;

  return ShowdesktopHandlerWindowInterface::PostPaintAction::Damage;
}

void ShowdesktopHandler::PaintOpacity (unsigned short &opacity)
{
  if (progress_ == 0.0f)
    opacity = std::numeric_limits <unsigned short>::max();
  else
    opacity *= (1.0f - progress_);
}

unsigned int ShowdesktopHandler::GetPaintMask()
{
  return (progress_ == 1.0f) ? showdesktop_handler_window_interface_->NoCoreInstanceMask() : 0;
}

void ShowdesktopHandler::HandleShapeEvent()
{
  /* Ignore sent events from the InputRemover */
  if (remover_)
  {
    remover_->refresh();
  }
}

void ShowdesktopHandler::WindowFocusChangeNotify()
{
  if (showdesktop_handler_window_interface_->Minimized())
  {
    for (ShowdesktopHandlerWindowInterface *w : animating_windows)
      w->DisableFocus();

    showdesktop_handler_window_interface_->MoveFocusAway();

    for (ShowdesktopHandlerWindowInterface *w : animating_windows)
      w->EnableFocus();
  }
}

void ShowdesktopHandler::UpdateFrameRegion (CompRegion &r)
{
  r = CompRegion();

  /* Ensure no other plugins can touch this frame region */
  showdesktop_handler_window_interface_->OverrideFrameRegion (r);
}

}

