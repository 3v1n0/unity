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

#include <gio/gio.h>

#include "inputremover.h"

namespace unity
{

class ShowdesktopHandlerWindowInterface
{
  public:

    enum class PostPaintAction {
      Wait = 0,
      Damage = 1,
      Remove = 2
    };

    virtual ~ShowdesktopHandlerWindowInterface ();

    void EnableFocus () { DoEnableFocus (); }
    void DisableFocus () { DoDisableFocus (); }

    bool OverrideRedirect () { return IsOverrideRedirect (); }
    bool Managed () { return IsManaged (); }
    bool Grabbed () { return IsGrabbed (); }
    bool DesktopOrDock () { return IsDesktopOrDock (); }
    bool SkipTaskbarOrPager () { return IsSkipTaskbarOrPager (); }
    bool Hidden () { return IsHidden (); }
    bool Shaded () { return IsShaded (); }
    bool Minimized () { return IsMinimized (); }
    bool ShowDesktopMode () { return IsInShowdesktopMode (); }
    void OverrideFrameRegion (CompRegion &r) { return DoOverrideFrameRegion (r); }

    void Hide () { DoHide (); }
    void NotifyHidden () { DoNotifyHidden (); }
    void Show () { DoShow (); }
    void NotifyShown () { DoNotifyShown (); }
    void MoveFocusAway () { DoMoveFocusAway (); }

    PostPaintAction HandleAnimations (unsigned int ms) { return DoHandleAnimations (ms); }
    void AddDamage () { DoAddDamage (); }

    void DeleteHandler () { DoDeleteHandler (); }

    unsigned int NoCoreInstanceMask () { return GetNoCoreInstanceMask (); }

  private:

    virtual void DoEnableFocus () = 0;
    virtual void DoDisableFocus () = 0;

    virtual bool IsOverrideRedirect () = 0;
    virtual bool IsManaged () = 0;
    virtual bool IsGrabbed () = 0;
    virtual bool IsDesktopOrDock () = 0;

    virtual bool IsSkipTaskbarOrPager () = 0;
    virtual bool IsHidden () = 0;
    virtual bool IsInShowdesktopMode () = 0;
    virtual bool IsShaded () = 0;

    virtual bool IsMinimized () = 0;

    virtual void DoOverrideFrameRegion (CompRegion &) = 0;

    virtual void DoHide () = 0;
    virtual void DoNotifyHidden () = 0;
    virtual void DoShow () = 0;
    virtual void DoNotifyShown () = 0;

    virtual void DoMoveFocusAway () = 0;
    virtual PostPaintAction DoHandleAnimations (unsigned int ms) = 0;
    virtual void DoAddDamage () = 0;

    virtual void DoDeleteHandler () = 0;

    virtual unsigned int GetNoCoreInstanceMask () = 0;
};

class ShowdesktopHandler
{
 public:

  ShowdesktopHandler (ShowdesktopHandlerWindowInterface *uwi, compiz::WindowInputRemoverLockAcquireInterface *lock_acquire_interface);
  ~ShowdesktopHandler ();

  typedef enum {
    StateVisible = 0,
    StateFadeOut = 1,
    StateFadeIn = 2,
    StateInvisible = 3
  } State;

public:

  void FadeOut ();
  void FadeIn ();
  ShowdesktopHandlerWindowInterface::PostPaintAction Animate (unsigned int ms);
  void PaintOpacity (unsigned short &opacity);
  unsigned int GetPaintMask ();
  void HandleShapeEvent ();
  void WindowFocusChangeNotify ();
  void UpdateFrameRegion (CompRegion &r);

  ShowdesktopHandler::State GetState ();

  static const unsigned int fade_time;
  static std::list <ShowdesktopHandlerWindowInterface *> animating_windows;
  static bool ShouldHide (ShowdesktopHandlerWindowInterface *);
  static void InhibitLeaveShowdesktopMode (guint32 xid);
  static void AllowLeaveShowdesktopMode (guint32 xid);
  static guint32 InhibitingXid ();

private:

  ShowdesktopHandlerWindowInterface *showdesktop_handler_window_interface_;
  compiz::WindowInputRemoverLockAcquireInterface *lock_acquire_interface_;
  compiz::WindowInputRemoverLock::Ptr remover_;
  ShowdesktopHandler::State         state_;
  float                                  progress_;
  bool                                   was_hidden_;
  static guint32		         inhibiting_xid;
};

}


#endif
