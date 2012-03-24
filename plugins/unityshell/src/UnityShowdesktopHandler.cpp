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

bool UnityShowdesktopHandler::shouldHide (UnityShowdesktopHandlerWindowInterface *wi)
{
  if (wi->overrideRedirect ())
    return false;

  if (!wi->managed ())
    return false;

  if (wi->grabbed ())
    return false;

  if (wi->desktopOrDock ())
   return false;

  if (wi->skipTaskbarOrPager ())
    return false;

  if (wi->hidden ())
    if (!(wi->showDesktopMode () || wi->shaded ()))
      return false;

  return true;
}

guint32 UnityShowdesktopHandler::mInhibitingXid = 0;

void
UnityShowdesktopHandler::inhibitLeaveShowdesktopMode (guint32 xid)
{
  if (!mInhibitingXid)
    mInhibitingXid = xid;
}

void
UnityShowdesktopHandler::allowLeaveShowdesktopMode (guint32 xid)
{
  if (mInhibitingXid == xid)
    mInhibitingXid = 0;
}

guint32
UnityShowdesktopHandler::inhibitingXid ()
{
  return mInhibitingXid;
}

UnityShowdesktopHandler::UnityShowdesktopHandler (UnityShowdesktopHandlerWindowInterface *wi) :
  mShowdesktopHandlerWindowInterface (wi),
  mRemover (wi->inputRemover ()),
  mState (Visible),
  mProgress (0.0f)
{
}

UnityShowdesktopHandler::~UnityShowdesktopHandler ()
{
  if (mRemover)
    delete mRemover;
}

void UnityShowdesktopHandler::fadeOut ()
{
  mState = UnityShowdesktopHandler::FadeOut;
  mProgress = 0.0f;

  mWasHidden = mShowdesktopHandlerWindowInterface->hidden ();

  if (!mWasHidden)
  {
    mShowdesktopHandlerWindowInterface->hide ();
    mShowdesktopHandlerWindowInterface->notifyHidden ();
    mRemover->save ();
    mRemover->remove ();
  }

  if (std::find (animating_windows.begin(),
                 animating_windows.end(),
                 mShowdesktopHandlerWindowInterface) == animating_windows.end())
    animating_windows.push_back (mShowdesktopHandlerWindowInterface);
}

void UnityShowdesktopHandler::fadeIn ()
{
  mState = UnityShowdesktopHandler::FadeIn;

  if (!mWasHidden)
  {
    mShowdesktopHandlerWindowInterface->show ();
    mShowdesktopHandlerWindowInterface->notifyShown ();
    mRemover->restore ();
  }

  if (std::find (animating_windows.begin(),
                 animating_windows.end(),
                 mShowdesktopHandlerWindowInterface) == animating_windows.end())
    animating_windows.push_back(mShowdesktopHandlerWindowInterface);
}

UnityShowdesktopHandlerWindowInterface::PostPaintAction UnityShowdesktopHandler::animate (unsigned int ms)
{
  float inc = ms / static_cast <float> (fade_time);

  if (mState == UnityShowdesktopHandler::FadeOut)
  {
    mProgress += inc;
    if (mProgress >= 1.0f)
    {
      mProgress = 1.0f;
      mState = Invisible;
    }
  }
  else if (mState == FadeIn)
  {
    mProgress -= inc;
    if (mProgress <= 0.0f)
    {
      mProgress = 0.0f;
      mState = Visible;
    }
  }
  else if (mState == Visible)
    return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Remove;
  else if (mState == Invisible)
    return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Wait;

  return UnityShowdesktopHandlerWindowInterface::PostPaintAction::Damage;
}

void UnityShowdesktopHandler::paintOpacity (unsigned short &opacity)
{
  if (mProgress == 1.0f || mProgress == 0.0f)
    opacity = std::numeric_limits <unsigned short>::max ();
  else
    opacity *= (1.0f - mProgress);
}

unsigned int UnityShowdesktopHandler::getPaintMask ()
{
  return (mProgress == 1.0f) ? mShowdesktopHandlerWindowInterface->noCoreInstanceMask () : 0;
}

void UnityShowdesktopHandler::handleShapeEvent ()
{
  /* Ignore sent events from the InputRemover */
  if (mRemover)
  {
    mRemover->save ();
    mRemover->remove ();
  }
}

void UnityShowdesktopHandler::windowFocusChangeNotify ()
{
  if (mShowdesktopHandlerWindowInterface->minimized ())
  {
    for (UnityShowdesktopHandlerWindowInterface *w : animating_windows)
      w->disableFocus ();

    mShowdesktopHandlerWindowInterface->moveFocusAway ();

    for (UnityShowdesktopHandlerWindowInterface *w : animating_windows)
      w->enableFocus ();
  }
}

void UnityShowdesktopHandler::updateFrameRegion (CompRegion &r)
{
  r = CompRegion ();

  /* Ensure no other plugins can touch this frame region */
  mShowdesktopHandlerWindowInterface->overrideFrameRegion (r);
}

}

