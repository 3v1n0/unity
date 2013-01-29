// -*- Mode: C++; indent-tabs-mode: nil; tab-width: 2 -*-
/*
 * Copyright (C) 2011 Canonical Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authored By:
 * Sam Spilsbury <sam.spilsbury@canonical.com>
 */

#ifndef _COMPIZ_COMPIZMINIMIZEDWINDOWHANDLER_H
#define _COMPIZ_COMPIZMINIMIZEDWINDOWHANDLER_H

#include <core/core.h>
#include "minimizedwindowhandler.h"
#include "comptransientfor.h"
#include <memory>

// Will be merged back into compiz
namespace compiz {

class PrivateCompizMinimizedWindowHandler
{
public:

  PrivateCompizMinimizedWindowHandler () {};

  CompWindow         *mWindow;
};

template <typename Screen, typename Window>
class CompizMinimizedWindowHandler:
    public MinimizedWindowHandler
{
public:

  CompizMinimizedWindowHandler (CompWindow *w, compiz::WindowInputRemoverLockAcquireInterface *lock_acquire);
  ~CompizMinimizedWindowHandler ();

  void setVisibility (bool visible);
  unsigned int getPaintMask ();

  void minimize ();
  void unminimize ();

  void updateFrameRegion (CompRegion &r);

  void windowNotify (CompWindowNotify n);

  static void setFunctions (bool keepMinimized);
  static void handleCompizEvent (const char *, const char *, CompOption::Vector &);
  static void handleEvent (XEvent *event);
  static std::list<CompWindow *> minimizingWindows;

  typedef CompizMinimizedWindowHandler<Screen, Window> Type;
  typedef std::list <Type *> List;
protected:

  std::vector<unsigned int> getTransients ();

private:

  PrivateCompizMinimizedWindowHandler *priv;
  static bool handleEvents;
  static List minimizedWindows;
};
}

/* XXX minimizedWindows should be removed because it is dangerous to keep
 *     a list of windows separate to compiz-core. The list could get out of
 *     sync and cause more crashes like LP: #918329, LP: #864758.
 */
template <typename Screen, typename Window>
typename compiz::CompizMinimizedWindowHandler<Screen, Window>::List compiz::CompizMinimizedWindowHandler<Screen, Window>::minimizedWindows;

template <typename Screen, typename Window>
CompWindowList compiz::CompizMinimizedWindowHandler<Screen, Window>::minimizingWindows;

template <typename Screen, typename Window>
bool compiz::CompizMinimizedWindowHandler<Screen, Window>::handleEvents = true;

template <typename Screen, typename Window>
compiz::CompizMinimizedWindowHandler<Screen, Window>::CompizMinimizedWindowHandler(CompWindow *w, compiz::WindowInputRemoverLockAcquireInterface *lock_acquire) :
  MinimizedWindowHandler (screen->dpy(), w->id(), lock_acquire)
{
  priv = new PrivateCompizMinimizedWindowHandler();

  priv->mWindow = w;

}

template <typename Screen, typename Window>
compiz::CompizMinimizedWindowHandler<Screen, Window>::~CompizMinimizedWindowHandler ()
{
  minimizingWindows.remove (priv->mWindow);
  minimizedWindows.remove (this);
}

template <typename Screen, typename Window>
std::vector<unsigned int>
compiz::CompizMinimizedWindowHandler<Screen, Window>::getTransients ()
{
  std::vector <unsigned int> transients;

  for (CompWindow *w : screen->windows())
  {
    compiz::CompTransientForReader reader (w);

    if (reader.isTransientFor (priv->mWindow->id()) ||
        reader.isGroupTransientFor (priv->mWindow->id()))
      transients.push_back (w->id());
  }

  return transients;
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::setVisibility (bool visible)
{
  MinimizedWindowHandler::setVisibility (visible, priv->mWindow->id());

  CompositeWindow::get (priv->mWindow)->addDamage();
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::minimize ()
{
  Atom          wmState = XInternAtom (screen->dpy(), "WM_STATE", 0);
  unsigned long data[2];

  std::vector<unsigned int> transients = getTransients();

  handleEvents = true;
  priv->mWindow->windowNotify (CompWindowNotifyMinimize);
  priv->mWindow->changeState (priv->mWindow->state() | CompWindowStateHiddenMask);

  minimizedWindows.push_back (this);

  for (unsigned int &w : transients)
  {
    CompWindow *win = screen->findWindow (w);

    if (win && win->isMapped())
    {
      Window *w = Window::get (win);
      if (!w->mMinimizeHandler)
        w->mMinimizeHandler.reset (new Type (win, w));
      w->mMinimizeHandler->minimize();
    }
  }

  priv->mWindow->windowNotify (CompWindowNotifyHide);
  setVisibility (false);

  data[0] = IconicState;
  data[1] = None;

  XChangeProperty (screen->dpy(), priv->mWindow->id(), wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);

  priv->mWindow->changeState (priv->mWindow->state() | CompWindowStateHiddenMask);

  /* Don't allow other windows to be focused by moveInputFocusToOtherWindow */
  for (auto mh : minimizedWindows)
    mh->priv->mWindow->focusSetEnabled (Window::get (mh->priv->mWindow), false);

  priv->mWindow->moveInputFocusToOtherWindow();

  for (auto mh : minimizedWindows)
    mh->priv->mWindow->focusSetEnabled (Window::get (mh->priv->mWindow), true);
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::windowNotify (CompWindowNotify n)
{
  if (n == CompWindowNotifyFocusChange && priv->mWindow->minimized())
  {
    for (auto mh : minimizedWindows)
      mh->priv->mWindow->focusSetEnabled (Window::get (mh->priv->mWindow), false);

    priv->mWindow->moveInputFocusToOtherWindow();

    for (auto mh : minimizedWindows)
      mh->priv->mWindow->focusSetEnabled (Window::get (mh->priv->mWindow), true);
  }
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::updateFrameRegion (CompRegion &r)
{
  unsigned int oldUpdateFrameRegionIndex;
  r = CompRegion();

  /* Ensure no other plugins can touch this frame region */
  oldUpdateFrameRegionIndex = priv->mWindow->updateFrameRegionGetCurrentIndex();
  priv->mWindow->updateFrameRegionSetCurrentIndex (MAXSHORT);
  priv->mWindow->updateFrameRegion (r);
  priv->mWindow->updateFrameRegionSetCurrentIndex (oldUpdateFrameRegionIndex);
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::unminimize()
{
  Atom          wmState = XInternAtom (screen->dpy(), "WM_STATE", 0);
  unsigned long data[2];

  std::vector<unsigned int> transients = getTransients();

  minimizedWindows.remove (this);

  priv->mWindow->focusSetEnabled (Window::get (priv->mWindow), true);

  priv->mWindow->windowNotify (CompWindowNotifyUnminimize);
  priv->mWindow->changeState (priv->mWindow->state() & ~CompWindowStateHiddenMask);
  priv->mWindow->windowNotify (CompWindowNotifyShow);

  for (unsigned int &w : transients)
  {
    CompWindow *win = screen->findWindow (w);

    if (win && win->isMapped())
    {
      Window *w = Window::get (win);
      if (w && w->mMinimizeHandler)
      {
	w->mMinimizeHandler->unminimize();
	w->mMinimizeHandler.reset();
      }
    }
  }

  setVisibility (true);

  priv->mWindow->changeState (priv->mWindow->state() & ~CompWindowStateHiddenMask);

  data[0] = NormalState;
  data[1] = None;

  XChangeProperty (screen->dpy(), priv->mWindow->id(), wmState, wmState,
                   32, PropModeReplace, (unsigned char *) data, 2);
}

template <typename Screen, typename Window>
unsigned int
compiz::CompizMinimizedWindowHandler<Screen, Window>::getPaintMask ()
{
  bool doMask = true;

  for (CompWindow *w : minimizingWindows)
  {
    if (w->id() == priv->mWindow->id())
      doMask = false;
    break;
  }

  if (doMask)
    return PAINT_WINDOW_NO_CORE_INSTANCE_MASK;

  return 0;
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::handleCompizEvent (const char *pluginName,
                                                                         const char *eventName,
                                                                         CompOption::Vector &o)
{
  if (!handleEvents)
    return;

  if (strncmp (pluginName, "animation", 9) == 0 &&
      strncmp (eventName, "window_animation", 16) == 0)
  {
    if (CompOption::getStringOptionNamed (o, "type", "") == "minimize")
    {
      CompWindow *w = screen->findWindow (CompOption::getIntOptionNamed (
                                            o, "window", 0));
      if (w)
      {
        if (CompOption::getBoolOptionNamed (o, "active", false))
          minimizingWindows.push_back (w);
        else
        {
          /* Ugly hack for LP#977189 */
	  CompositeWindow::get (w)->release();
	  GLWindow::get (w)->release();
          minimizingWindows.remove (w);
        }
      }
    }
  }

  if (!CompOption::getBoolOptionNamed (o, "active", false) &&
      minimizingWindows.empty())
  {
    handleEvents = false;
  }
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::handleEvent (XEvent *event)
{
  /* Ignore sent events from the InputRemover */
  if (screen->XShape() && event->type ==
      screen->shapeEvent() + ShapeNotify &&
      !event->xany.send_event)
  {
    CompWindow *w = screen->findWindow (((XShapeEvent *) event)->window);

    if (w)
    {
      Window *pw = Window::get (w);
      Type *compizMinimizeHandler = pw->mMinimizeHandler.get();

      /* Restore and re-save input shape and remove */
      if (compizMinimizeHandler)
      {
        compizMinimizeHandler->setVisibility (true);
        compizMinimizeHandler->setVisibility (false);
      }
    }
  }
}

template <typename Screen, typename Window>
void
compiz::CompizMinimizedWindowHandler<Screen, Window>::setFunctions (bool keepMinimized)
{
  for (CompWindow *w : screen->windows())
  {
    bool m = w->minimized();
    bool enable = keepMinimized && w->mapNum() > 0;

    if (m)
      w->unminimize();
    w->minimizeSetEnabled (Window::get (w), enable);
    w->unminimizeSetEnabled (Window::get (w), enable);
    w->minimizedSetEnabled (Window::get (w), enable);
    if (m)
      Window::get (w)->window->minimize();
  }
}

#endif
