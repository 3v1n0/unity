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
#ifndef UNITY_SHOWDESKTOP_HANDLER_H
#define UNITY_SHOWDESKTOP_HANDLER_H

#include <list>
#include <algorithm>
#include <core/region.h>

#include "inputremover.h"

namespace unity
{

class UnityShowdesktopHandlerWindowInterface
{
  public:

    enum class PostPaintAction {
      Wait = 0,
      Damage = 1,
      Remove = 2
    };

    virtual ~UnityShowdesktopHandlerWindowInterface ();

    void enableFocus () { doEnableFocus (); }
    void disableFocus () { doDisableFocus (); }

    bool overrideRedirect () { return isOverrideRedirect (); }
    bool managed () { return isManaged (); }
    bool grabbed () { return isGrabbed (); }
    bool desktopOrDock () { return isDesktopOrDock (); }
    bool skipTaskbarOrPager () { return isSkipTaskbarOrPager (); }
    bool hidden () { return isHidden (); }
    bool shaded () { return isShaded (); }
    bool minimized () { return isMinimized (); }
    bool showDesktopMode () { return isInShowdesktopMode (); }
    void overrideFrameRegion (CompRegion &r) { return doOverrideFrameRegion (r); }

    void hide () { doHide (); }
    void notifyHidden () { doNotifyHidden (); }
    void show () { doShow (); }
    void notifyShown () { doNotifyShown (); }
    void moveFocusAway () { doMoveFocusAway (); }

    PostPaintAction handleAnimations (unsigned int ms) { return doHandleAnimations (ms); }
    void addDamage () { doAddDamage (); }

    void deleteHandler () { doDeleteHandler (); }

    unsigned int noCoreInstanceMask () { return getNoCoreInstanceMask (); }

    compiz::WindowInputRemoverInterface * inputRemover () { return getInputRemover (); }

  protected:

    virtual void doEnableFocus () = 0;
    virtual void doDisableFocus () = 0;

    virtual bool isOverrideRedirect () = 0;
    virtual bool isManaged () = 0;
    virtual bool isGrabbed () = 0;
    virtual bool isDesktopOrDock () = 0;

    virtual bool isSkipTaskbarOrPager () = 0;
    virtual bool isHidden () = 0;
    virtual bool isInShowdesktopMode () = 0;
    virtual bool isShaded () = 0;

    virtual bool isMinimized () = 0;

    virtual void doOverrideFrameRegion (CompRegion &) = 0;

    virtual void doHide () = 0;
    virtual void doNotifyHidden () = 0;
    virtual void doShow () = 0;
    virtual void doNotifyShown () = 0;

    virtual void doMoveFocusAway () = 0;
    virtual PostPaintAction doHandleAnimations (unsigned int ms) = 0;
    virtual void doAddDamage () = 0;

    virtual void doDeleteHandler () = 0;

    virtual unsigned int getNoCoreInstanceMask () = 0;

    virtual compiz::WindowInputRemoverInterface * getInputRemover () = 0;
};

class UnityShowdesktopHandler
{
 public:

  UnityShowdesktopHandler (UnityShowdesktopHandlerWindowInterface *uwi);
  ~UnityShowdesktopHandler ();

  typedef enum {
    Visible = 0,
    FadeOut = 1,
    FadeIn = 2,
    Invisible = 3
  } State;

public:

  void fadeOut ();
  void fadeIn ();
  UnityShowdesktopHandlerWindowInterface::PostPaintAction animate (unsigned int ms);
  void paintOpacity (unsigned short &opacity);
  unsigned int getPaintMask ();
  void handleShapeEvent ();
  void windowFocusChangeNotify ();
  void updateFrameRegion (CompRegion &r);

  UnityShowdesktopHandler::State state ();

  static const unsigned int fade_time;
  static std::list <UnityShowdesktopHandlerWindowInterface *> animating_windows;
  static bool shouldHide (UnityShowdesktopHandlerWindowInterface *);
  static void inhibitLeaveShowdesktopMode (guint32 xid);
  static void allowLeaveShowdesktopMode (guint32 xid);
  static guint32 inhibitingXid ();

private:

  UnityShowdesktopHandlerWindowInterface *mShowdesktopHandlerWindowInterface;
  compiz::WindowInputRemoverInterface    *mRemover;
  UnityShowdesktopHandler::State         mState;
  float                                  mProgress;
  bool                                   mWasHidden;
  static guint32		                     mInhibitingXid;
};

}


#endif
