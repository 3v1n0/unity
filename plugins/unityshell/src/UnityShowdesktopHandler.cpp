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

namespace unity
{

UnityShowdesktopHandlerWindowInterface::~UnityShowdesktopHandlerWindowInterface ()
{
}

/* 300 ms */
const unsigned int UnityShowdesktopHandler::fade_time = 300;
std::list <UnityShowdesktopHandlerWindowInterface *> UnityShowdesktopHandler::animating_windows (0);

bool UnityShowdesktopHandler::ShouldHide (UnityShowdesktopHandlerWindowInterface *wi)
{
  if (wi->OverrideRedirect ())
    return false;

  if (!wi->Managed ())
    return false;

  if (wi->Grabbed ())
    return false;

  if (wi->DesktopOrDock ())
   return false;

  if (wi->SkipTaskbarOrPager ())
    return false;

  if (wi->Hidden ())
    if ((wi->ShowDesktopMode () || wi->Shaded ()))
      return false;

  return true;
}

guint32 UnityShowdesktopHandler::inhibiting_xid = 0;

void
UnityShowdesktopHandler::InhibitLeaveShowdesktopMode (guint32 xid)
{
  if (!inhibiting_xid)
    inhibiting_xid = xid;
}

void
UnityShowdesktopHandler::AllowLeaveShowdesktopMode (guint32 xid)
{
  if (inhibiting_xid == xid)
    inhibiting_xid = 0;
}

guint32
UnityShowdesktopHandler::InhibitingXid ()
{
  return inhibiting_xid;
}

UnityShowdesktopHandler::UnityShowdesktopHandler (UnityShowdesktopHandlerWindowInterface *wi) :
  showdesktop_handler_window_interface_ (wi),
  remover_ (wi->InputRemover ()),
  state_ (StateVisible),
  progress_ (0.0f)
{
}

UnityShowdesktopHandler::~UnityShowdesktopHandler ()
{
}

void UnityShowdesktopHandler::FadeOut ()
{
  if (state_ != StateVisible && state_ != StateFadeIn)
    return;

  state_ = UnityShowdesktopHandler::StateFadeOut;
  progress_ = 0.0f;

  was_hidden_ = showdesktop_handler_window_interface_->Hidden ();

  if (!was_hidden_)
  {
    showdesktop_handler_window_interface_->Hide ();
    showdesktop_handler_window_interface_->NotifyHidden ();
    remover_->save ();
    remover_->remove ();

    if (std::find (animating_windows.begin(),
                   animating_windows.end(),
		   showdesktop_handler_window_interface_) == animating_windows.end())
      animating_windows.push_back (showdesktop_handler_window_interface_);

  }
}

void UnityShowdesktopHandler::FadeIn ()
{
  if (state_ != StateInvisible && state_ != StateFadeOut)
    return;

  state_ = UnityShowdesktopHandler::StateFadeIn;

  if (!was_hidden_)
  {
    showdesktop_handler_window_interface_->Show ();
    showdesktop_handler_window_interface_->NotifyShown ();
    remover_->restore ();

    if (std::find (animating_windows.begin(),
                   animating_windows.end(),
		   showdesktop_handler_window_interface_) == animating_windows.end())
      animating_windows.push_back(showdesktop_handler_window_interface_);
  }
}

UnityShowdesktopHandlerWindowInterface::PostPaintAction UnityShowdesktopHandler::Animate (unsigned int ms)
{
  float inc = ms / static_cast <float> (fade_time);

  if (state_ == UnityShowdesktopHandler::StateFadeOut)
  {
    progress_ += inc;
    if (progress_ >= 1.0f)
    {
      progress_ = 1.0f;
      state_ = StateInvisible;
    }
  }
  else if (state_ == StateFadeIn)
  {
    progress_ -= inc;
    if (progress_ <= 0.0f)
    {
      progress_ = 0.0f;
      state_ = StateVisible;
    }
  }
  else if (state_ == StateVisible)
    return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Remove;
  else if (state_ == StateInvisible)
    return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Wait;

  return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Damage;
}

void UnityShowdesktopHandler::PaintOpacity (unsigned short &opacity)
{
  if (progress_ == 1.0f || progress_ == 0.0f)
    opacity = std::numeric_limits <unsigned short>::max ();
  else
    opacity *= (1.0f - progress_);
}

unsigned int UnityShowdesktopHandler::GetPaintMask ()
{
  return (progress_ == 1.0f) ? showdesktop_handler_window_interface_->NoCoreInstanceMask () : 0;
}

void UnityShowdesktopHandler::HandleShapeEvent ()
{
  /* Ignore sent events from the InputRemover */
  if (remover_)
  {
    remover_->save ();
    remover_->remove ();
  }
}

void UnityShowdesktopHandler::WindowFocusChangeNotify ()
{
  if (showdesktop_handler_window_interface_->Minimized ())
  {
    for (UnityShowdesktopHandlerWindowInterface *w : animating_windows)
      w->DisableFocus ();

    showdesktop_handler_window_interface_->MoveFocusAway ();

    for (UnityShowdesktopHandlerWindowInterface *w : animating_windows)
      w->EnableFocus ();
  }
}

void UnityShowdesktopHandler::UpdateFrameRegion (CompRegion &r)
{
  r = CompRegion ();

  /* Ensure no other plugins can touch this frame region */
  showdesktop_handler_window_interface_->OverrideFrameRegion (r);
}

}

